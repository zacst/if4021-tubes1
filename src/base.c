#include "simlib.h"             /* Required for use of simlib.c */

#define EVENT_ARRIVAL_CAFETERIA         1 /* Event type for arrival at cafeteria */
#define EVENT_DEPARTURE_CAFETERIA         2 /* Event type for arrival */
#define EVENT_ARRIVAL_HOT_FOOD         3 /* Event type for arrival */
#define EVENT_DEPARTURE_HOT_FOOD         4 /* Event type for arrival */
#define EVENT_ARRIVAL_SPECIALTY_SANDWICHES         5 /* Event type for arrival */
#define EVENT_DEPARTURE_SPECIALTY_SANDWICHES         6 /* Event type for arrival */
#define EVENT_ARRIVAL_DRINKS                7 /* Event type for arrival at drink station */
#define EVENT_ARRIVAL_CASHIER                8 /* Event type for arrival at drink station */
#define EVENT_DEPARTURE_CASHIER                9 /* Event type for arrival at drink station */

/* Generate space for maximum number of use case */
#define LIST_QUEUE_CAFETERIA 1 /* 1 cafeteria queue */
#define LIST_QUEUE_HOT_FOOD 2 /* 2 hot food queue */
#define LIST_WORKER_HOT_FOOD 4 /* 2 hot food worker */
#define LIST_QUEUE_SPECIALTY_SANDWICHES 6 /* 2 specialty sandwiches queue */
#define LIST_WORKER_SPECIALTY_SANDWICHES 8 /* 2 specialty sandwiches worker */
#define LIST_QUEUE_CASHIER 10 /* 3 cashier queue */
#define LIST_CASHIER 13 /* 3 cashier */

#define STREAM_INTERARRIVAL 1       /* Random-number stream for interarrivals */
#define STREAM_HOT_FOOD_ST 2   /* Random-number stream for hot food service times */
#define STREAM_HOT_FOOD_ACT 3   /* Random-number stream for hot food accumulated cashier times */
#define STREAM_SPECIALTY_SANDWICHES_ST 4 /* Random-number stream for specialy sandwiches service times */
#define STREAM_SPECIALTY_SANDWICHES_ACT 5 /* Random-number stream for specialy sandwiches accumulated cashier times */
#define STREAM_DRINKS_ST 6 /* Random-number stream for drinks service times */
#define STREAM_DRINKS_ACT 7 /* Random-number stream for drinks service times */
#define STREAM_NUM_CUSTOMERS 8 /* Random-number stream for amount of customers */
#define STREAM_ROUTE 9 /* Random-number stream for route (hot food, specialty sandwiches, drinks) */

/* Declare non-simlib global variables. */
int min_hot_food_st, max_hot_food_st, min_hot_food_act, max_hot_food_act;
int min_specialty_sandwiches_st, max_specialty_sandwiches_st, min_specialty_sandwiches_act, max_specialty_sandwiches_act;
int min_drinks_st, max_drinks_st, min_drinks_act, max_drinks_act;

float mean_interarrival;

/* Hyperparameters */
int num_hot_food_worker, num_specialty_sandwiches_worker, num_cashier;

/* Probability Variables */
double prob_distrib_num_customers[4];

int num_customers;

double prob_distrib_routes[3];

int route;

double random_time;

/* Helper Functions */

/* Handle the route random generator */
int map_route_int_to_event_type(int route_int) {
    switch (route_int) {
        case 1:
            return EVENT_ARRIVAL_HOT_FOOD;
        case 2:
            return EVENT_ARRIVAL_SPECIALTY_SANDWICHES;
        case 3:
            return EVENT_ARRIVAL_DRINKS;
    }
}

// Base case: 1 hot food, 1 specialty sandwich, 2 cashier

void init_model(void) /* Initialization Function */
{
    /* Receive Hyperparameter Input */
    printf("Number of Hot Food Workers: ");
    scanf("%d", &num_hot_food_worker);
    printf("Number of Specialty Sandwiches Workers: ");
    scanf("%d", &num_specialty_sandwiches_worker);
    printf("Number of Cashiers: ");
    scanf("%d", &num_cashier);

    /* Constants */
    min_hot_food_st = 50; max_hot_food_st = 120;
    min_hot_food_act = 20; max_hot_food_act = 40;

    min_specialty_sandwiches_st = 60; max_specialty_sandwiches_st = 180;
    min_specialty_sandwiches_act = 5; max_specialty_sandwiches_act = 15;

    min_drinks_st = 5; max_drinks_st = 20;
    min_drinks_act = 5; max_drinks_act = 10;

    mean_interarrival = 30;

    /* Probability Variables */
    prob_distrib_num_customers[1] = 0.5;
    prob_distrib_num_customers[2] = 0.8;
    prob_distrib_num_customers[3] = 0.9;
    prob_distrib_num_customers[4] = 1;
    
    prob_distrib_routes[1] = 0.8;
    prob_distrib_routes[2] = 0.95;
    prob_distrib_routes[3] = 1;

    /* Determine the number of customers */
    num_customers = random_integer(prob_distrib_num_customers, STREAM_NUM_CUSTOMERS);

    /* Schedules the next customer(s) */
    random_time = expon(mean_interarrival, STREAM_INTERARRIVAL);
    for (int i = 0 ; i < num_customers ; i++)
    {
        event_schedule(sim_time + random_time, EVENT_ARRIVAL_CAFETERIA);
    }
}

void arrive_cafeteria(void) /* Arrival at Cafeteria */
{
    /* Current Customer */

    /* Determine the customer's next destination based on probability */
    route = random_integer(prob_distrib_routes, STREAM_ROUTE);

    /* Schedule the customer's next route */
    /* Assumes no walking time is needed */
    event_schedule(sim_time, map_route_int_to_event_type(route));

    /* Next Customer(s) */

    /* Determine the number of customers */
    num_customers = random_integer(prob_distrib_num_customers, STREAM_NUM_CUSTOMERS);

    /* Schedules the next customer(s) */
    random_time = expon(mean_interarrival, STREAM_INTERARRIVAL);
    for (int i = 0 ; i < num_customers ; i++)
    {
        event_schedule(sim_time + random_time, EVENT_ARRIVAL_CAFETERIA);
    }
}

void arrive_hot_food(void) /* Arrival at Hot Food */
{   
    /* Initializes the least amount of customers in a queue to the first one */
    int least_queue_length = list_size[LIST_QUEUE_HOT_FOOD];

    /* Checks the availability of all workers based on the number of hot food workers */
    int i = 0;
    while (i < num_hot_food_worker) {
        /* Finds and inserts on the first available worker */
        if (list_size[LIST_WORKER_HOT_FOOD + i] == 0) {
            /* Worker is idle, so start service by inserting dummy record */
            list_file(FIRST, LIST_WORKER_HOT_FOOD + i);

            /* Schedules time for departure with additional service time */
            /* ACT time accumulated later on at the cashier */
            event_schedule(sim_time + uniform(min_hot_food_st, max_hot_food_st, STREAM_HOT_FOOD_ST), EVENT_DEPARTURE_HOT_FOOD);

            /* Ends the search because a free worker has been found */
            return;
        }

        /* Takes note of queue with least amount of workers*/
        if (list_size[LIST_WORKER_HOT_FOOD + i] < least_queue_length) {
            least_queue_length = list_size[LIST_WORKER_HOT_FOOD + i];
        }

        i++;
    }

    /* If there are no available workers, assign to the least */
    transfer[1] = sim_time;
    list_file(LAST, LIST_QUEUE_HOT_FOOD + i);
}

void arrive_specialty_sandwiches(void) /* Arrival at Specialty Sandwiches */
{
    /* Initializes the least amount of customers in a queue to the first one */
    int least_queue_length = list_size[LIST_QUEUE_SPECIALTY_SANDWICHES];

    /* Checks the availability of all workers based on the number of hot food workers */
    int i = 0;
    while (i < num_specialty_sandwiches_worker) {
        /* Finds the first available worker */
        if (list_size[LIST_WORKER_SPECIALTY_SANDWICHES + i] == 0) {
            /* Worker is idle, so start service by inserting dummy record */
            list_file(FIRST, LIST_WORKER_SPECIALTY_SANDWICHES + i);

            /* Schedules time for departure with additional service time */
            /* ACT time accumulated later on at the cashier */
            event_schedule(sim_time + uniform(min_specialty_sandwiches_st, max_specialty_sandwiches_st, STREAM_SPECIALTY_SANDWICHES_ST), EVENT_DEPARTURE_SPECIALTY_SANDWICHES);

            /* Ends the search because a free worker has been found */
            return;
        }

        /* Takes note of queue with least amount of workers*/
        if (list_size[LIST_WORKER_SPECIALTY_SANDWICHES + i] < least_queue_length) {
            least_queue_length = list_size[LIST_WORKER_SPECIALTY_SANDWICHES + i];
        }

        i++;
    }

    /* If there are no available workers, assign to the least */
    transfer[1] = sim_time;
    list_file(LAST, LIST_QUEUE_SPECIALTY_SANDWICHES + i);
}

void arrive_drinks(void) /* Arrival at Drinks */
{
    /* No queue, so just schedule the next event to the cashier */
    event_schedule(sim_time + uniform(min_drinks_st, max_drinks_st, STREAM_DRINKS_ST), EVENT_ARRIVAL_CASHIER);
}

void arrive_cashier(void) /* Arrival at Cashier */
{
    /* Initializes the least amount of customers in a queue to the first one */
    int least_queue_length = list_size[LIST_CASHIER];

    /* Checks the availability of all cashiers based on the number of cashiers */
    int i = 0;
    while (i < num_cashier) {
        /* Check to see whether cashier is busy */
        if (list_size[LIST_CASHIER + i] == 1) {
            /* Cashier is busy, insert into queue */
            transfer[1] = sim_time;
            list_file(LAST, LIST_QUEUE_CASHIER + i);
        }
        /* Finds and inserts on the first available cashier */
        if (list_size[LIST_CASHIER + i] == 0) {
            /* Cashier is idle, so start service by inserting dummy record */
            list_file(FIRST, LIST_CASHIER + i);

            /* ACT time accumulated later at the cashier */

            /* Schedules time for departure with additional service time */            
            event_schedule(sim_time + uniform(min_hot_food_st, max_hot_food_st, STREAM_HOT_FOOD_ST), EVENT_DEPARTURE_CASHIER);

            /* Ends the search because a free worker has been found */
            return;
        }

        /* Takes note of queue with least amount of workers*/
        if (list_size[LIST_WORKER_HOT_FOOD + i] < least_queue_length) {
            least_queue_length = list_size[LIST_WORKER_HOT_FOOD + i];
        }

        i++;
    }

    /* If there are no available workers, assign to the least */
    transfer[1] = sim_time;
    list_file(LAST, LIST_QUEUE_HOT_FOOD + i);

    /* Fixes */
    /* 1. Departure from cashier? When does it end? */
    /* 2. Waiting time at cashier uses the time finished by previous person */
    /* 3. ACT time must be recorded at each route */
}

void depart_cashier(void)
{
    /* worker index stored in transfer[3] when the service was scheduled */
    int cashier_idx = (int) transfer[3];

    /* remove the customer from cashier (this also sets transfer[] to record) */
    list_remove(FIRST, LIST_CASHIER + cashier_idx);
    /* customer leaves system here — could collect statistics using sim_time & arrival times if desired */

    /* if there is someone waiting in queue, start their service */
    if (list_size[LIST_QUEUE_CASHIER + cashier_idx] > 0) {
        /* dequeue next customer into transfer[] */
        list_remove(FIRST, LIST_QUEUE_CASHIER + cashier_idx);
        /* transfer[2] should contain that customer's ACT (as set when they were enqueued) */
        transfer[3] = (double) cashier_idx;
        list_file(FIRST, LIST_CASHIER + cashier_idx);
        double service_time = transfer[2];
        if (service_time <= 0.0) service_time = uniform(min_drinks_st, max_drinks_st, STREAM_DRINKS_ST);
        event_schedule(sim_time + service_time, EVENT_DEPARTURE_CASHIER);
    } else {
        /* cashier now idle (no record in LIST_CASHIER) */
    }
}

void depart_hot_food(void) /* Depart from Hot Food */
{
    /* Check if queue is empty */
    if (list_size[LIST_QUEUE_HOT_FOOD] == 0) {
        /* Remove the customer from worker list */
        list_remove(FIRST, LIST_WORKER_HOT_FOOD);
    }
    /* Queue is not empty */
    else {
        /* Remove the first customer from queue */
        list_remove(FIRST, LIST_QUEUE_HOT_FOOD);
        /* Schedules to Drinks */
        event_schedule(sim_time, EVENT_ARRIVAL_DRINKS);
    }
}

void depart_specialty_sandwiches(void) /* Depart from Specialty Sandwiches */
{
    /* Check if queue is empty */
    if (list_size[LIST_QUEUE_SPECIALTY_SANDWICHES] == 0) {
        /* Remove the customer from worker list */
        list_remove(FIRST, LIST_WORKER_SPECIALTY_SANDWICHES);
    }
    /* Queue is not empty */
    else {
        /* Remove the first customer from queue */
        list_remove(FIRST, LIST_QUEUE_SPECIALTY_SANDWICHES);
        /* Schedules to Drinks */
        event_schedule(sim_time, EVENT_ARRIVAL_DRINKS);
    }
}

int main() /* Main function. */
{
    /* Set maxatr = max(maximum number of attributes per record, 4) */

    maxatr = 4;  /* NEVER SET maxatr TO BE SMALLER THAN 4. */
    maxlist = 20;  // ini apa njir kalau nilainya terlalu kecil error tapi terlalu besar juga error

    /* Initialize simlib */

    init_simlib();

    /* Initialize the model. */

    init_model();

    /* Service for 90 minutes */
    while (sim_time <= 5400) {

        /* Determine and move to next event. */
        timing();

        /* Invoke the appropriate event function. */
        switch (next_event_type) {
            case EVENT_ARRIVAL_CAFETERIA:              arrive_cafeteria(); break;
            case EVENT_ARRIVAL_HOT_FOOD:               arrive_hot_food(); break;
            case EVENT_DEPARTURE_HOT_FOOD:             depart_hot_food(); break;
            case EVENT_ARRIVAL_SPECIALTY_SANDWICHES:   arrive_specialty_sandwiches(); break;
            case EVENT_DEPARTURE_SPECIALTY_SANDWICHES: depart_specialty_sandwiches(); break;
            case EVENT_ARRIVAL_DRINKS:                 arrive_drinks(); break;
            case EVENT_ARRIVAL_CASHIER:                arrive_cashier(); break;
            case EVENT_DEPARTURE_CASHIER:              depart_cashier(); break;
        }
    }
}