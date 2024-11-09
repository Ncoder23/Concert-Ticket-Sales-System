#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>
#include<string.h>
// #include<malloc.h>
#include<pthread.h>
#include"utility.h"
#define HP_COUNT 1
#define MP_COUNT 3
#define LP_COUNT 6
#define TOTAL_COUNT (HP_COUNT + MP_COUNT + LP_COUNT)
#define TOTAL_ROW 10
#define TOTAL_COL 10
#define simulation_duration 60

// Global Variable
int simulation_time;
int N = 5;
char seat_matrix[TOTAL_ROW][TOTAL_COL][5];
int at1[15] = {0}, st1[15] = {0}, tat1[15] = {0}, bt1[15]={0}, rt1[15]={0};
float throughput[3] = {0};
int response_time[3] = {0}, turn_around_time[3] = {0}; // per seller type 0:L, 1:M, 2:H.

float avg_rt=0, avg_tat=0, num_cust_served = 0;
// Thread Variable
pthread_t seller_t[TOTAL_COUNT];
pthread_mutex_t thread_count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t thread_waiting_for_clock_tick_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t reservation_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t thread_completion_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t condition_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  condition_cond  = PTHREAD_COND_INITIALIZER;

// Function Defination
void display_queue(queue *q);
void create_seller_threads(pthread_t *thread, char seller_type, int no_of_sellers);
void current_time_slice_wait();
void wakeup_all_seller_threads();
void *sell(void *);
queue * generate_customer_queue(int);
int compare_by_arrival_time(void * data1, void * data2);
int find_available_seat(char seller_type);

int thread_count = 0;
int threads_waiting_for_clock_tick = 0;
int active_thread = 0;
int verbose = 0;

void create_seller_threads(pthread_t *thread, char seller_type, int no_of_sellers){
	// Create all threads
	for(int t_no = 0; t_no < no_of_sellers; t_no++) {
		sell_arg *seller_arg = (sell_arg *) malloc(sizeof(sell_arg));
		seller_arg->seller_no = t_no;
		seller_arg->seller_type = seller_type;
		seller_arg->seller_queue = generate_customer_queue(N);

		pthread_mutex_lock(&thread_count_mutex);
		thread_count++;
		pthread_mutex_unlock(&thread_count_mutex);
		if(verbose)
			printf("Creating thread %c%02d\n",seller_type,t_no);
		pthread_create(thread+t_no, NULL, &sell, seller_arg);
	}
}

void display_queue(queue *q) {
	for(node *ptr = q->head;ptr!=NULL;ptr=ptr->next) {
		customer *cust = (customer * )ptr->data;
		printf("[%d,%d]",cust->cust_no,cust->arrival_time);
	}
}

void current_time_slice_wait(){
	// Check if all threads has finished their jobs for this time slice
	while(1){
		pthread_mutex_lock(&thread_waiting_for_clock_tick_mutex);
		if(threads_waiting_for_clock_tick == active_thread) {
			threads_waiting_for_clock_tick = 0;	
			pthread_mutex_unlock(&thread_waiting_for_clock_tick_mutex);
			break;
		}
		pthread_mutex_unlock(&thread_waiting_for_clock_tick_mutex);
	}
}
void wakeup_all_seller_threads() {

	pthread_mutex_lock( &condition_mutex );
	if(verbose)
		printf("00:%02d Main Thread Broadcasting Clock Tick\n",simulation_time);
	pthread_cond_broadcast( &condition_cond);
	pthread_mutex_unlock( &condition_mutex );
}

void *sell(void *t_args) {
	// Initializing thread
	sell_arg *args = (sell_arg *) t_args;
	queue * customer_queue = args->seller_queue;
	queue * seller_queue = create_queue();
	char seller_type = args->seller_type;
	int seller_no = args->seller_no + 1;
	
	pthread_mutex_lock(&thread_count_mutex);
	thread_count--;
	active_thread++;
	pthread_mutex_unlock(&thread_count_mutex);

	customer *cust = NULL;
	int random_wait_time = 0;
    int temp1 = 0;


	while(simulation_time < simulation_duration) {
		// Waiting for clock tick
		pthread_mutex_lock(&condition_mutex);
		if(verbose)
			printf("00:%02d %c%02d Waiting for next clock tick\n",simulation_time,seller_type,seller_no);
		
		pthread_mutex_lock(&thread_waiting_for_clock_tick_mutex);
		threads_waiting_for_clock_tick++;
		pthread_mutex_unlock(&thread_waiting_for_clock_tick_mutex);
		
		pthread_cond_wait( &condition_cond, &condition_mutex);
		if(verbose)
			printf("00:%02d %c%02d Received Clock Tick\n",simulation_time,seller_type,seller_no);
		pthread_mutex_unlock( &condition_mutex );

		//  Sell
		if(simulation_time == simulation_duration) break;
		// All New Customer Came
		while(customer_queue->size > 0 && ((customer *)customer_queue->head->data)->arrival_time <= simulation_time) {
			customer *temp = (customer *) dequeue (customer_queue);
			enqueue(seller_queue,temp);
			printf("00:%02d %c%d Customer No %c%d%02d arrived\n",simulation_time,seller_type,seller_no,seller_type,seller_no,temp->cust_no);
		}
		// Serve next customer
		if(cust == NULL && seller_queue->size>0) {
			cust = (customer *) dequeue(seller_queue);
			printf("00:%02d %c%d Serving Customer No %c%d%02d\n",simulation_time,seller_type,seller_no,seller_type,seller_no,cust->cust_no);
			switch(seller_type) {
				case 'H':
					random_wait_time = (rand()%2) + 1;
					response_time[2] += (simulation_time - cust->arrival_time);
					break;
				case 'M':
					random_wait_time = (rand()%3) + 2;
					response_time[1] += (simulation_time - cust->arrival_time);
					break;
				case 'L':
					random_wait_time = (rand()%4) + 4;
					response_time[0] += (simulation_time - cust->arrival_time);
					break;
			}
		}
		if(cust != NULL) {
			if(random_wait_time == 0) {
				// Selling Seat
				pthread_mutex_lock(&reservation_mutex);
				//  Find seat
				int seatIndex = find_available_seat(seller_type);
				if(seatIndex == -1) {
					printf("00:%02d %c%d Customer No %c%d%02d has been told Concert Sold Out.\n",simulation_time,seller_type,seller_no,seller_type,seller_no,cust->cust_no);
				} else {
					int row_no = seatIndex/TOTAL_COL;
					int col_no = seatIndex%TOTAL_COL;
					sprintf(seat_matrix[row_no][col_no],"%c%d%02d",seller_type,seller_no,cust->cust_no);
					printf("00:%02d %c%d Customer No %c%d%02d assigned seat %d,%d \n",simulation_time,seller_type,seller_no,seller_type,seller_no,cust->cust_no,row_no+1,col_no+1);
                    num_cust_served++;
					switch (seller_type)
					{
					case 'L':
						throughput[0]++;
						turn_around_time[0] += (simulation_time - cust->arrival_time);
						break;
					case 'M':
						throughput[1]++;
						turn_around_time[1] += (simulation_time - cust->arrival_time);
						break;
					case 'H':
						throughput[2]++;
						turn_around_time[2] += (simulation_time - cust->arrival_time);
						break;
					}
				}
				pthread_mutex_unlock(&reservation_mutex);
				cust = NULL;
			} else {
				random_wait_time--;
			}
		}
	}

	while(cust!=NULL || seller_queue->size > 0) {
		if(cust==NULL)
			cust = (customer *) dequeue(seller_queue);
		printf("00:%02d %c%d Ticket Sale Closed. Customer No %c%d%02d Leaves\n",simulation_time,seller_type,seller_no,seller_type,seller_no,cust->cust_no);
		cust = NULL;
	}
	pthread_mutex_lock(&thread_count_mutex);
	active_thread--;
	pthread_mutex_unlock(&thread_count_mutex);
    return NULL;
}

int find_available_seat(char seller_type){
	int seatIndex = -1;

	if(seller_type == 'H') {
		for(int row_no = 0;row_no < TOTAL_ROW; row_no ++ ){
			for(int col_no = 0;col_no < TOTAL_COL; col_no ++) {
				if(strcmp(seat_matrix[row_no][col_no],"-") == 0) {
					seatIndex = row_no * TOTAL_COL + col_no;
					return seatIndex;
				}
			}
		}
	} else if(seller_type == 'M') {
		int mid = TOTAL_ROW / 2;
		int row_jump = 0;
		for(row_jump = 0;;row_jump++) {
			int row_no = mid+row_jump;
			if(mid + row_jump < TOTAL_ROW) {
				for(int col_no = 0;col_no < TOTAL_COL; col_no ++) {
					if(strcmp(seat_matrix[row_no][col_no],"-") == 0) {
						seatIndex = row_no * TOTAL_COL + col_no;
						return seatIndex;
					}
				}
			}
			row_no = mid - row_jump;
			if(mid - row_jump >= 0) {
				for(int col_no = 0;col_no < TOTAL_COL; col_no ++) {
					if(strcmp(seat_matrix[row_no][col_no],"-") == 0) {
						seatIndex = row_no * TOTAL_COL + col_no;
						return seatIndex;
					}
				}
			}
			if(mid + row_jump >= TOTAL_ROW && mid - row_jump < 0) {
				break;
			}
		}
	} else if(seller_type == 'L') {
		for(int row_no = TOTAL_ROW - 1;row_no >= 0; row_no -- ){
			for(int col_no = TOTAL_COL - 1;col_no >= 0; col_no --) {
				if(strcmp(seat_matrix[row_no][col_no],"-") == 0) {
					seatIndex = row_no * TOTAL_COL + col_no;
					return seatIndex;
				}
			}
		}
	}

	return -1;
}

queue * generate_customer_queue(int N){
	queue * customer_queue = create_queue();
	char cust_no = 0;
	while(N--) {
		customer *cust = (customer *) malloc(sizeof(customer));
		cust->cust_no = cust_no;
		cust->arrival_time = rand() % simulation_duration;
        at1[cust_no] = cust->arrival_time;
		enqueue(customer_queue,cust);
		cust_no++;
	}
	sort(customer_queue, compare_by_arrival_time);
	node * ptr = customer_queue->head;
	cust_no = 0;
	while(ptr!=NULL) {
		cust_no ++;
		customer *cust = (customer *) ptr->data;
		cust->cust_no = cust_no;
		ptr = ptr->next;
	}
	return customer_queue;
}

int compare_by_arrival_time(void * data1, void * data2) {
	customer *c1 = (customer *)data1;
	customer *c2 = (customer *)data2;
	if(c1->arrival_time < c2->arrival_time) {
		return -1;
	} else if(c1->arrival_time == c2->arrival_time){
		return 0;
	} else {
		return 1;
	}
}

int main(int argc, char** argv) {
    srand(4388);

    if(argc == 2) {
        N = atoi(argv[1]);
    }
    // Initialize Global Variables
    for(int r=0; r<TOTAL_ROW; r++) {
        for(int c=0; c<TOTAL_COL; c++) {
            strncpy(seat_matrix[r][c],"-",1);
        }
    }
    // Create all threads
    create_seller_threads(seller_t, 'H', HP_COUNT);
    create_seller_threads(seller_t + HP_COUNT, 'M', MP_COUNT);
    create_seller_threads(seller_t + HP_COUNT + MP_COUNT, 'L', LP_COUNT);

    // Wait for threads to finish initialization and wait for synchronized clock tick
    while(1) {
        pthread_mutex_lock(&thread_count_mutex);
        if(thread_count == 0) {
            pthread_mutex_unlock(&thread_count_mutex);
            break;
        }
        pthread_mutex_unlock(&thread_count_mutex);
    }

    // Simulate each time quanta/slice as one iteration
    printf("Starting Simulation\n");
    threads_waiting_for_clock_tick = 0;
    wakeup_all_seller_threads(); // For first tick

    do {

        // Wake up all thread
        current_time_slice_wait();
        simulation_time = simulation_time + 1;
        wakeup_all_seller_threads();
        // Wait for thread completion
    } while(simulation_time < simulation_duration);

    // Wakeup all thread so that no more thread keep waiting for clock Tick in limbo
    wakeup_all_seller_threads();

    while(active_thread);

    // Display concert chart
    printf("\n\n");
    printf("Final Concert Seat Chart\n");
    printf("========================\n");

    int h_customers = 0,m_customers = 0,l_customers = 0;
    for(int r=0;r<TOTAL_ROW;r++) {
        for(int c=0;c<TOTAL_COL;c++) {
            if(c!=0)
                printf("\t");
            printf("%5s",seat_matrix[r][c]);
            if(seat_matrix[r][c][0]=='H') h_customers++;
            if(seat_matrix[r][c][0]=='M') m_customers++;
            if(seat_matrix[r][c][0]=='L') l_customers++;
        }
        printf("\n");
    }

    printf("\n\nStat for N = %02d\n",N);
    printf("===============\n");
    printf(" ============================================\n");
    printf("|%3c | No of Customers | Got Seat | Returned |\n",' ');
    printf(" ============================================\n");
    printf("|%3c | %15d | %8d | %8d |\n",'H',HP_COUNT*N,h_customers,(HP_COUNT*N)-h_customers);
    printf("|%3c | %15d | %8d | %8d |\n",'M',MP_COUNT*N,m_customers,(MP_COUNT*N)-m_customers);
    printf("|%3c | %15d | %8d | %8d |\n",'L',LP_COUNT*N,l_customers,(LP_COUNT*N)-l_customers);
    printf(" ============================================\n");

	printf("\nAverage Response time of seller L is %.2f\n", response_time[0]/(N * 6.0));
    printf("Average Response time of seller M is %.2f\n", response_time[1]/(N * 3.0));
    printf("Average Response time of seller H is %.2f\n", response_time[2]/(N * 1.0));

	printf("\nAverage Turn Around time of seller L is %.2f\n", turn_around_time[0]/(N * 6.0));
    printf("Average Turn Around time of seller M is %.2f\n", turn_around_time[1]/(N * 3.0));
    printf("Average Turn around time of seller H is %.2f\n", turn_around_time[2]/(N * 1.0));

    printf("\nThroughput of seller L is %.2f\n", throughput[2]/60.0);
    printf("Throughput of seller M is %.2f\n", throughput[1]/60.0);
    printf("Throughput of seller H is %.2f\n", throughput[0]/60.0);

    return 0;
}
