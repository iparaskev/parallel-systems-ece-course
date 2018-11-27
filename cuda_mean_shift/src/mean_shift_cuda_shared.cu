#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <sys/time.h>

#define BLOCK_SIZE 16
#define stride 4
#define DIMS 2

/* overload atomicAdd for double */
#if !defined(__CUDA_ARCH__) || __CUDA_ARCH__ >= 600

#else
static __inline__ __device__ 
double atomicAdd(double *address, double val) 
{
		unsigned long long int* address_as_ull = (unsigned long long int*)address;
		unsigned long long int old = *address_as_ull, assumed;
		if (val==0.0)
			return __longlong_as_double(old);
		do {
			assumed = old;
			old = atomicCAS(address_as_ull, assumed, __double_as_longlong(val +__longlong_as_double(assumed)));
		} while (assumed != old);
		return __longlong_as_double(old);
}
#endif


// global variable
int rows, columns;
__device__ int dev_rows, dev_columns, dev_sparse_count = 0;
__device__ double dev_h, dev_norm;

// device functions 
__global__ void get_exp_shared(double *x, double *y, double *w);
__global__ void multiply_arrays(double *x, double *w, double *y_new);
__global__ void get_sum_per_row(double *w, double *sums);
__global__ void mean_shift(double *y_new, double *sums_array, double *y, double *norms);
__global__ void compute_norms(double *norms, double *norm_per_block);

// local cpu functions
double *read_data(const char *name);
int check_results(double *y, char *name);
void write_results(char **argv, int iterations, double time);

double
now()
{
        struct timeval tv;
        gettimeofday(&tv, 0);
        return tv.tv_sec + tv.tv_usec / 1000000.0;
}

int 
main(int argc, char **argv)
{
        double *x, *y, h, epsilon, norm, *sums_array;
        double *dev_x, *dev_y, *dev_w, *dev_y_new, *dev_sums_array, *dev_norms;
        double *dev_blocks_norm; 

	    if (argc < 6){
	    	fprintf(stderr, "it needs 5 arguments, which are: h iterations data validation_data dims\n");
	    	exit(1);
	    }
	    // initialize arrays at the host
	    int iterations = atoi(argv[2]);
	    columns = atoi(argv[5]);
	    x = read_data(argv[3]);
	    y = (double *) malloc(sizeof *y * rows * columns);
	    memcpy(y, x, sizeof(double) * rows * columns);
	    sums_array = (double *) malloc(sizeof *sums_array * rows);
	    h = atoi(argv[1]);
	    epsilon = 1e-4*h;
	    
	    // parameters for reduction per block for the computation of norm
	    int blocks_size = 512;
	    int thread_size = blocks_size;
	    double *blocks_norm = (double *) malloc(sizeof *blocks_norm * (blocks_size / 2));
	    
	    cudaError_t error;
	    // initialize at the device
	    error = cudaMalloc((void **) &dev_x, rows * columns * sizeof(double));
	    if (error)
	    	printf("malloc dev_x %s\n", cudaGetErrorString(error));
	    
	    error = cudaMalloc((void **) &dev_y, rows * columns * sizeof(double));
	    if (error)
	    	printf("malloc dev_y %s\n", cudaGetErrorString(error));
	    
	    error = cudaMalloc((void **) &dev_y_new, rows * columns * sizeof(double));
	    if (error)
	    	printf("malloc dev_y_new %s\n", cudaGetErrorString(error));
	    
	    error = cudaMalloc((void **) &dev_sums_array, rows * sizeof(double));
	    if (error)
	    	printf("malloc dev_sums_array %s\n" ,cudaGetErrorString(error));
	    
	    error = cudaMalloc((void **) &dev_w, (rows * rows) * sizeof(double));
	    
	    // allocate max memory for sparse
	    int paranom = 2;
	    while (error){
	    	error = cudaMalloc((void **) &dev_w, (rows * rows) / paranom++ * sizeof(double));
	    	if (paranom > 20){
	    		fprintf(stderr, "%s\n", "no memory");
	    		return 1;
	    	}
	    }
	    	
	    error = cudaMalloc((void **) &dev_norms, blocks_size * thread_size * sizeof(double));
	    if (error)
	    	printf("malloc dev_norms %s\n", cudaGetErrorString(error));
	    
	    error = cudaMalloc((void **) &dev_blocks_norm, (blocks_size / 2) * sizeof(double));
	    if (error)
	    	printf("malloc dev_blocks_norm %s\n", cudaGetErrorString(error));
	    
	    // copy initial arrays from host to device
	    cudaMemcpy(dev_x, x, rows * columns * sizeof(double), cudaMemcpyHostToDevice);
	    cudaMemcpy(dev_y, y, rows * columns * sizeof(double), cudaMemcpyHostToDevice);

	    cudaMemcpyToSymbol(dev_rows, &rows, sizeof(int));
	    cudaMemcpyToSymbol(dev_columns, &columns, sizeof(int));
	    cudaMemcpyToSymbol(dev_h, &h, sizeof(double));
	    
	    norm = INT_MAX;
	    double t_start = now(), time_passed;
	    
	    // for(int j = 0; j < iterations; j++){
	    int j = 0;
	    while(sqrt(norm) > epsilon){	
	    	norm = 0;
	    	
	    	get_exp_shared<<<256, BLOCK_SIZE * BLOCK_SIZE/stride>>>(dev_x, dev_y, dev_w);
	    	cudaDeviceSynchronize();
	    	
	    	// zero new array
	    	memset(y, 0, rows * columns * sizeof(double));
	    	cudaMemcpy(dev_y_new, y, rows * columns * sizeof(double), cudaMemcpyHostToDevice);
	    	
	    	multiply_arrays<<<256, 64>>>(dev_x, dev_w, dev_y_new);
	    	cudaDeviceSynchronize();
	    	
	    	// sum sparse
	    	memset(sums_array, 0, rows * sizeof(double));
	    	cudaMemcpy(dev_sums_array, sums_array, rows * sizeof(double), cudaMemcpyHostToDevice);
	    	get_sum_per_row<<<512, 512>>>(dev_w, dev_sums_array);
	    	cudaDeviceSynchronize();

	    	mean_shift<<<blocks_size, 512>>>(dev_y_new, dev_sums_array, dev_y, dev_norms);
	    	cudaDeviceSynchronize();
	    	
	    	compute_norms<<<blocks_size / 2, 512>>>(dev_norms, dev_blocks_norm);
	    	cudaDeviceSynchronize();
	    	cudaMemcpy(blocks_norm, dev_blocks_norm, (blocks_size / 2) * sizeof(double), cudaMemcpyDeviceToHost);
	    	for (int k = 0; k < (blocks_size/2); k++)
	    		norm += blocks_norm[k];
	    	
	    	printf("iteration %d error %f \n", j, sqrt(norm));
	    	// break;
	    	j++;
	    }
	    time_passed = now() - t_start;
	    printf("time passed %f\n", now() - t_start);
	    cudaMemcpy(y, dev_y, rows * columns * sizeof(double), cudaMemcpyDeviceToHost);
	    check_results(y, argv[4]);

	    FILE *f = fopen("res.txt", "w");
	    for (int i = 0; i < rows; i++){
	    	for (int j = 0; j < columns; j++)
	    		fprintf(f, "%f ", y[i * columns + j]);
	    	fprintf(f, "\n");
	    }
	    fclose(f);

	    // clean up gpu
	    cudaFree((void *) &dev_x);
	    cudaFree((void *) &dev_y);
	    cudaFree((void *) &dev_w);
	    cudaFree((void *) &dev_sums_array);
	    cudaFree((void *) &dev_norms);
	    cudaFree((void *) &dev_blocks_norm);
	    cudaFree((void *) &dev_y_new);

	    // clean up cpu
	    free(x);
	    free(y);
	    free(sums_array);

	    return 0;
}

__global__ void
multiply_arrays(double *x, double *w, double *y_new)
{
	double tmp = 0;
	
	int row, column;
	double dist;
	int global_thread_id = threadIdx.x + blockIdx.x * blockDim.x;

	int w_tid = (global_thread_id) * 3;
	while (w_tid < 3 * dev_sparse_count){
		row = w[w_tid];
		column = w[w_tid + 1];
		dist = w[w_tid + 2];

		// mulitply distance with every element in the row
		for(int i = 0; i < dev_columns; i++){
			tmp = dist * x[column * dev_columns + i];
			atomicAdd(&y_new[row * dev_columns + i], tmp);
		}
		w_tid += blockDim.x * gridDim.x * 3;
	}
}

__global__ void
get_sum_per_row(double *w, double *sums)
{
	int global_thread_id = threadIdx.x + blockIdx.x * blockDim.x;
	int row;
	double dist;

	int w_tid = (global_thread_id) * 3;
	while (w_tid < dev_sparse_count * 3){
		row = w[w_tid];
		dist = w[w_tid + 2];
		atomicAdd(&sums[row], dist);
		
		w_tid += blockDim.x * gridDim.x * 3;
	}
}

__global__ void
mean_shift(double *y_new, double *sums_array, double *y, double *norms)
{
	int y_tid = threadIdx.x + blockIdx.x * blockDim.x;
	int sum_tid = y_tid / dev_columns;
	double m_dif = 0;
	int norms_tid = y_tid;
	norms[norms_tid] = 0;
	dev_sparse_count	= 0;

	while (y_tid < (dev_rows * dev_columns)){
		y_new[y_tid] /= sums_array[sum_tid];
		m_dif = y_new[y_tid] - y[y_tid];
		norms[norms_tid] += pow(m_dif, 2);
		y[y_tid] = y_new[y_tid];

		y_tid += gridDim.x * blockDim.x;
		sum_tid = y_tid / dev_columns;
	}
}

__global__ void 
compute_norms(double *norms, double *norm_per_block)
{
	int n_tid = 2 * (threadIdx.x + blockIdx.x * blockDim.x);
	int i = 1;
	int initial_tid = n_tid / 2;
	int block_limit = gridDim.x * blockDim.x;

	int block_end = 2 * (blockIdx.x * blockDim.x + blockDim.x) - 1; 

	if (n_tid < (2 * block_limit)){
		
		while ( (i < (2 * blockDim.x)) && n_tid < block_end && 
					 (n_tid + i) <= block_end){
			
			norms[n_tid] += norms[n_tid + i];
			n_tid = n_tid + i * (initial_tid * 2 - 2 * (blockIdx.x * blockDim.x));
			i *= 2;
			__syncthreads();
			
		}

		
		if (!((initial_tid) % blockDim.x))
			norm_per_block[blockIdx.x] = norms[n_tid];
		
	} 
}

__device__ double
compute_distance(double *y_i, double *x_j, double limit)
{
	double dist = 0, tmp;
	for (int i = 0; i < dev_columns; i++){
		tmp = y_i[i] - x_j[i];
		dist += tmp * tmp;
		if (dist > limit)
			return 0;
	}

	return exp( -dist / (2 * limit));
}

__global__ void
get_exp_shared(double *x, double *y, double *w)
{
	
	double dist;
	double limit = dev_h * dev_h;
	long int pos;
	
	__shared__ double s_y[BLOCK_SIZE][DIMS];
	__shared__ double s_x[BLOCK_SIZE][DIMS];
	
	/* upper_bound is used so the division with BLOCK_SIZE 
		 will give the bigger real mulitplier of BLOCK_SIZE	*/
	long int global_block_id = blockIdx.x;
	long int upper_bound = dev_rows + BLOCK_SIZE - 1;

	/* we	want block_row to change every rows elements
		 and block col every one BLOCK_SIZE elemetns		 */
	long int block_row = blockIdx.x / (upper_bound / BLOCK_SIZE);
	long int block_col = blockIdx.x % (upper_bound / BLOCK_SIZE);

	long int y_tid = threadIdx.x / (BLOCK_SIZE / stride) + block_row * BLOCK_SIZE;
	long int x_tid = stride * (threadIdx.x % (BLOCK_SIZE / stride)) + block_col * BLOCK_SIZE;

	long int upload_col = (x_tid - block_col * BLOCK_SIZE) % DIMS;
	long int upload_col_x = y_tid % DIMS;

	long int s_y_tid = y_tid - block_row * BLOCK_SIZE;
	long int s_x_tid = x_tid - block_col * BLOCK_SIZE;
	double loop_limit = (upper_bound * upper_bound ) / (BLOCK_SIZE * BLOCK_SIZE);
	
	int last_index = (upper_bound / BLOCK_SIZE - 1);

	int flag_row = ((block_col == last_index) && (dev_rows - BLOCK_SIZE * last_index) < DIMS);
	int flag_col = (block_row == last_index) && (dev_rows - BLOCK_SIZE * last_index) < DIMS;
	// TODO: if dims >> blocksize && not rows%dims == 0 problem!!!
	while (global_block_id < loop_limit){
		
		if (y_tid < dev_rows && x_tid < dev_rows){
			__syncthreads();
			
			if ((flag_row && x_tid == (dev_rows - stride)) || (x_tid == (((block_col+1)*BLOCK_SIZE) - 1)) && DIMS > BLOCK_SIZE){
				for (int i = 0; i < (DIMS - upload_col); i++)
					s_y[s_y_tid][upload_col + i] = y[y_tid * dev_columns + upload_col + i];
			}
			else
				for (int i = 0; (i < stride && (upload_col + i) < dev_columns); i++)
					s_y[s_y_tid][upload_col + i] = y[y_tid * dev_columns + upload_col + i];
			
			if ((flag_col && y_tid == (dev_rows - 1)) || (y_tid == (((block_row+1)*BLOCK_SIZE) - 1) && DIMS > BLOCK_SIZE))
			 for (int j = 0; j < (DIMS - upload_col_x); j++)
				 for (int i = 0; i < stride; i++)
					s_x[s_x_tid + i][upload_col_x + j] = x[(x_tid + i) * dev_columns + upload_col_x + j];
			else
				for (int i = 0; i < stride; i++)
					s_x[s_x_tid + i][upload_col_x] = x[(x_tid + i) * dev_columns + upload_col_x];
			__syncthreads();
			
			for (int i = 0; i < stride; i++){
				
				dist = compute_distance(&s_y[s_y_tid][0], &s_x[s_x_tid][0], limit);
			 
				if (dist){

					pos = atomicAdd(&dev_sparse_count, 1);
						
					w[pos * 3] = y_tid;
					w[pos * 3 + 1] = x_tid;
					w[pos * 3 + 2] = dist;
				}
				s_x_tid++;
				x_tid++;
			}
		}

		global_block_id += gridDim.x;

		block_row = global_block_id / (upper_bound / BLOCK_SIZE);
		block_col = global_block_id % (upper_bound / BLOCK_SIZE);

		y_tid = threadIdx.x / (BLOCK_SIZE / stride) + block_row * BLOCK_SIZE;
		x_tid = stride * (threadIdx.x % (BLOCK_SIZE / stride)) + block_col * BLOCK_SIZE;

		s_y_tid = y_tid - block_row * BLOCK_SIZE;
		s_x_tid = x_tid - block_col * BLOCK_SIZE;

		flag_row = (block_col == last_index) && (dev_rows - BLOCK_SIZE * last_index) < DIMS;
		flag_col = (block_row == last_index) && (dev_rows - BLOCK_SIZE * last_index) < DIMS;
	}

}

double *
read_data(const char *name)
{
	FILE *f;
	f = fopen(name, "rb");
	fseek(f, 0L, SEEK_END);
	int pos = ftell(f);
	fseek(f, 0L, SEEK_SET);

	int number_elements = pos / sizeof(double);
	double *x = (double *) malloc(sizeof *x * number_elements);
	fread(x, sizeof *x, number_elements, f);
	rows = number_elements / columns;
	fclose(f);

	return x;
}

int 
check_results(double *y, char *name)
{
	double *mat_res = read_data(name);
	double dist = 0;
	for (int i = 0; i < rows; i++){
		for (int j = 0; j < columns; j++){
			dist += pow(y[i * columns + j] - mat_res[i * columns + j], 2);
			// if (dist > 1)
					// printf("%f %f %f\n", y[i * columns + j], mat_res[i * columns + j], dist);
		}
	}
	printf("done dist %.10f \n", dist);
	return 0;
}

void
write_results(char **argv, int iterations, double time)
{
	FILE *f;
	f = fopen("results_shared.txt", "a");
	fprintf(f, "%d %d %s %d %f\n", rows, columns,argv[1], iterations, time);
	fclose(f);
}
