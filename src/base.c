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

/* Customer type identifiers */
#define CUST_HOT_FOOD        1
#define CUST_SPECIALTY       2
#define CUST_DRINKS_ONLY     3

/* Declare non-simlib global variables. */
int min_hot_food_st, max_hot_food_st, min_hot_food_act, max_hot_food_act;
int min_specialty_sandwiches_st, max_specialty_sandwiches_st, min_specialty_sandwiches_act, max_specialty_sandwiches_act;
int min_drinks_st, max_drinks_st, min_drinks_act, max_drinks_act;

float mean_interarrival;

/* Hyperparameters */
int num_hot_food_worker, num_specialty_sandwiches_worker, num_cashier;

/* Probability Variables */
double prob_distrib_num_customers[5];

int num_customers;

double prob_distrib_routes[4];

int route;

double random_time;

/* Performance statistics */

/* Per-queue statistics */
float total_delay_hot_food = 0.0, total_delay_specialty = 0.0, total_delay_cashier = 0.0;
float num_delayed_hot_food = 0.0, num_delayed_specialty = 0.0, num_delayed_cashier = 0.0;
float max_delay_hot_food = 0.0, max_delay_specialty = 0.0, max_delay_cashier = 0.0;

float area_num_in_q_hot_food = 0.0, area_num_in_q_specialty = 0.0, area_num_in_q_cashier = 0.0;
float max_num_in_q_hot_food = 0.0, max_num_in_q_specialty = 0.0, max_num_in_q_cashier = 0.0;

/* Per-customer-type total delay */
float total_delay_type_hot = 0.0, total_delay_type_specialty = 0.0, total_delay_type_drink = 0.0;
float max_total_delay_type_hot = 0.0, max_total_delay_type_specialty = 0.0, max_total_delay_type_drink = 0.0;
int num_customers_hot = 0, num_customers_specialty = 0, num_customers_drink = 0;

/* Overall system tracking */
float area_total_in_system = 0.0;
float max_total_in_system = 0.0;
float total_customers = 0.0;
float time_last_event = 0.0;

/* Helper Functions */

/* Handle the route random generator */
int map_route_int_to_event_type(int route_int) {
    switch (route_int) {
        case 1:
            transfer[4] = CUST_HOT_FOOD;
            return EVENT_ARRIVAL_HOT_FOOD;
        case 2:
            transfer[4] = CUST_SPECIALTY;
            return EVENT_ARRIVAL_SPECIALTY_SANDWICHES;
        case 3:
            transfer[4] = CUST_DRINKS_ONLY;
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

    /* Initial schedule */
    double first_arrival_time = expon(mean_interarrival, STREAM_INTERARRIVAL);
    event_schedule(sim_time + first_arrival_time, EVENT_ARRIVAL_CAFETERIA);
}

void arrive_cafeteria(void) /* Arrival at Cafeteria */
{
    /* --- Generate a batch of customers arriving together --- */
    int batch_size = random_integer(prob_distrib_num_customers, STREAM_NUM_CUSTOMERS);
    for (int i = 0; i < batch_size; i++) {
        /* Decide the route for this customer */
        int route = random_integer(prob_distrib_routes, STREAM_ROUTE);
        int next_event = map_route_int_to_event_type(route);

        /* Schedule their immediate next step */
        event_schedule(sim_time, next_event);
    }

    /* Schedule the next batch of arrivals */
    double next_arrival_time = expon(mean_interarrival, STREAM_INTERARRIVAL);
    event_schedule(sim_time + next_arrival_time, EVENT_ARRIVAL_CAFETERIA);
}

void arrive_hot_food(void)
{
    int chosen_worker = 0;
    int least_queue_length = list_size[LIST_QUEUE_HOT_FOOD];

    /* Try to find an idle worker first */
    for (int i = 0; i < num_hot_food_worker; i++) {
        if (list_size[LIST_WORKER_HOT_FOOD + i] == 0) {
            /* Worker i is idle → start service immediately */
            transfer[1] = sim_time;                     /* arrival time */
            transfer[3] = (double)i;                            /* worker index */
            transfer[4] = CUST_HOT_FOOD;                /* customer type */
            list_file(FIRST, LIST_WORKER_HOT_FOOD + i);

            double service_time = uniform(min_hot_food_st, max_hot_food_st, STREAM_HOT_FOOD_ST);
            event_schedule(sim_time + service_time, EVENT_DEPARTURE_HOT_FOOD);
            return;
        }

        /* Track the worker with the smallest queue */
        if (list_size[LIST_QUEUE_HOT_FOOD + i] < least_queue_length) {
            least_queue_length = list_size[LIST_QUEUE_HOT_FOOD + i];
            chosen_worker = i;
        }
    }

    /* All busy → join the shortest queue */
    transfer[1] = sim_time;
    transfer[4] = CUST_HOT_FOOD;
    list_file(LAST, LIST_QUEUE_HOT_FOOD + chosen_worker);
}

void arrive_specialty_sandwiches(void)
{
    int chosen_worker = 0;
    int least_queue_length = list_size[LIST_QUEUE_SPECIALTY_SANDWICHES];

    /* Try to find an idle worker first */
    for (int i = 0; i < num_specialty_sandwiches_worker; i++) {
        if (list_size[LIST_WORKER_SPECIALTY_SANDWICHES + i] == 0) {
            /* Worker i is idle → start service immediately */
            transfer[1] = sim_time;
            transfer[3] = (double)i;
            transfer[4] = CUST_SPECIALTY;
            list_file(FIRST, LIST_WORKER_SPECIALTY_SANDWICHES + i);

            double service_time = uniform(min_specialty_sandwiches_st, max_specialty_sandwiches_st, STREAM_SPECIALTY_SANDWICHES_ST);
            event_schedule(sim_time + service_time, EVENT_DEPARTURE_SPECIALTY_SANDWICHES);
            return;
        }

        /* Track the worker with the smallest queue */
        if (list_size[LIST_QUEUE_SPECIALTY_SANDWICHES + i] < least_queue_length) {
            least_queue_length = list_size[LIST_QUEUE_SPECIALTY_SANDWICHES + i];
            chosen_worker = i;
        }
    }

    /* All busy → join the shortest queue */
    transfer[1] = sim_time;
    transfer[4] = CUST_SPECIALTY;
    list_file(LAST, LIST_QUEUE_SPECIALTY_SANDWICHES + chosen_worker);
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
            if (list_size[LIST_QUEUE_CASHIER + i] > max_num_in_q_cashier)
                max_num_in_q_cashier = list_size[LIST_QUEUE_CASHIER + i];
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
        if (list_size[LIST_QUEUE_CASHIER + i] < least_queue_length) {
            least_queue_length = list_size[LIST_QUEUE_CASHIER + i];
        }

        i++;
    }

    /* If there are no available workers, assign to the least */
    transfer[1] = sim_time;
    list_file(LAST, LIST_QUEUE_CASHIER + i);
}

void depart_cashier(void)
{
    int cashier_idx = (int) transfer[3];

    /* Remove customer from cashier */
    if (list_size[LIST_CASHIER + cashier_idx] > 0)
        list_remove(FIRST, LIST_CASHIER + cashier_idx);

    /* Compute delay: if transfer[1] holds arrival time at cashier */
    float delay = sim_time - transfer[1];
    total_delay_cashier += delay;
    num_delayed_cashier++;
    if (delay > max_delay_cashier)
        max_delay_cashier = delay;

    /* --- Always increment per-type customer count --- */
    int cust_type = (int) transfer[4];
    if (cust_type == CUST_HOT_FOOD) {
        total_delay_type_hot += delay;
        if (delay > max_total_delay_type_hot)
            max_total_delay_type_hot = delay;
    } else if (cust_type == CUST_SPECIALTY) {
        total_delay_type_specialty += delay;
        if (delay > max_total_delay_type_specialty)
            max_total_delay_type_specialty = delay;
    } else if (cust_type == CUST_DRINKS_ONLY) {
        total_delay_type_drink += delay;
        num_customers_drink++;
        if (delay > max_total_delay_type_drink)
            max_total_delay_type_drink = delay;
    }

    /* Start service for next customer in queue if any */
    if (list_size[LIST_QUEUE_CASHIER + cashier_idx] > 0) {
        list_remove(FIRST, LIST_QUEUE_CASHIER + cashier_idx);
        transfer[3] = (double) cashier_idx;
        list_file(FIRST, LIST_CASHIER + cashier_idx);

        double service_time = transfer[2];
        if (service_time <= 0.0) service_time = uniform(min_drinks_st, max_drinks_st, STREAM_DRINKS_ST);
        event_schedule(sim_time + service_time, EVENT_DEPARTURE_CASHIER);
    }
}

void depart_hot_food(void)
{
    /* worker index stored in transfer[3] when the service was scheduled */
    int worker_idx = (int) transfer[3];

    /* remove the just-finished customer from that worker’s list */
    if (list_size[LIST_WORKER_HOT_FOOD + worker_idx] > 0)
        list_remove(FIRST, LIST_WORKER_HOT_FOOD + worker_idx);

    /* if there is someone waiting in queue, start their service */
    if (list_size[LIST_QUEUE_HOT_FOOD + worker_idx] > 0) {
        /* dequeue next customer into transfer[] */
        list_remove(FIRST, LIST_QUEUE_HOT_FOOD + worker_idx);

        /* compute delay */
        float delay = sim_time - transfer[1];
        total_delay_hot_food += delay;
        num_delayed_hot_food++;
        if (delay > max_delay_hot_food) max_delay_hot_food = delay;

        /* accumulate per-type stats */
        total_delay_type_hot += delay;
        if (delay > max_total_delay_type_hot)
            max_total_delay_type_hot = delay;
        num_customers_hot++;

        if (list_size[LIST_QUEUE_HOT_FOOD + worker_idx] > max_num_in_q_hot_food)
            max_num_in_q_hot_food = list_size[LIST_QUEUE_HOT_FOOD + worker_idx];

        /* start service for that worker again */
        transfer[3] = (double) worker_idx;
        list_file(FIRST, LIST_WORKER_HOT_FOOD + worker_idx);
        double service_time = uniform(min_hot_food_st, max_hot_food_st, STREAM_HOT_FOOD_ST);
        event_schedule(sim_time + service_time, EVENT_DEPARTURE_HOT_FOOD);
    } else {
        /* no one waiting — worker now idle */
    }

    /* finished customer now goes to drinks */
    event_schedule(sim_time, EVENT_ARRIVAL_DRINKS);
}

void depart_specialty_sandwiches(void)
{
    int worker_idx = (int) transfer[3];

    if (list_size[LIST_WORKER_SPECIALTY_SANDWICHES + worker_idx] > 0)
        list_remove(FIRST, LIST_WORKER_SPECIALTY_SANDWICHES + worker_idx);

    if (list_size[LIST_QUEUE_SPECIALTY_SANDWICHES + worker_idx] > 0) {
        list_remove(FIRST, LIST_QUEUE_SPECIALTY_SANDWICHES + worker_idx);

        float delay = sim_time - transfer[1];
        total_delay_specialty += delay;
        num_delayed_specialty++;
        if (delay > max_delay_specialty) max_delay_specialty = delay;

        total_delay_type_specialty += delay;
        if (delay > max_total_delay_type_specialty)
            max_total_delay_type_specialty = delay;
        num_customers_specialty++;

        if (list_size[LIST_QUEUE_SPECIALTY_SANDWICHES + worker_idx] > max_num_in_q_specialty)
            max_num_in_q_specialty = list_size[LIST_QUEUE_SPECIALTY_SANDWICHES + worker_idx];

        transfer[3] = (double) worker_idx;
        list_file(FIRST, LIST_WORKER_SPECIALTY_SANDWICHES + worker_idx);
        double service_time = uniform(min_specialty_sandwiches_st, max_specialty_sandwiches_st, STREAM_SPECIALTY_SANDWICHES_ST);
        event_schedule(sim_time + service_time, EVENT_DEPARTURE_SPECIALTY_SANDWICHES);
    } else {
        /* worker idle */
    }

    /* finished customer goes to drinks */
    event_schedule(sim_time, EVENT_ARRIVAL_DRINKS);
}

int main() /* Main function. */
{
    /* Set maxatr = max(maximum number of attributes per record, 4) */

    maxatr = 5;  // ada tambahin transfer[4] jadinya 5
    maxlist = 30;

    /* Initialize simlib */

    init_simlib();

    /* Initialize the model. */

    init_model();

    /* Service for 90 minutes */
    while (sim_time <= 5400) {

        /* Determine and move to next event. */
        timing();

        /* --- Time since last event --- */
        float time_since_last_event = sim_time - time_last_event;
        time_last_event = sim_time;

        /* --- Update area accumulators for queues --- */
        area_num_in_q_hot_food += list_size[LIST_QUEUE_HOT_FOOD] * time_since_last_event;
        area_num_in_q_specialty += list_size[LIST_QUEUE_SPECIALTY_SANDWICHES] * time_since_last_event;

        /* For cashier, sum across all cashiers (assuming 0..num_cashier-1) */
        int total_cashier_queue = 0;
        for (int i = 0; i < num_cashier; i++)
            total_cashier_queue += list_size[LIST_QUEUE_CASHIER + i];
        area_num_in_q_cashier += total_cashier_queue * time_since_last_event;

        /* --- Update system-level stats --- */
        int total_in_system = 0;
        for (int i = 0; i < num_hot_food_worker; i++)
            total_in_system += list_size[LIST_WORKER_HOT_FOOD + i] + list_size[LIST_QUEUE_HOT_FOOD + i];
        for (int i = 0; i < num_specialty_sandwiches_worker; i++)
            total_in_system += list_size[LIST_WORKER_SPECIALTY_SANDWICHES + i] + list_size[LIST_QUEUE_SPECIALTY_SANDWICHES + i];
        for (int i = 0; i < num_cashier; i++)
            total_in_system += list_size[LIST_CASHIER + i] + list_size[LIST_QUEUE_CASHIER + i];
        area_total_in_system += total_in_system * time_since_last_event;
        if (total_in_system > max_total_in_system)
            max_total_in_system = total_in_system;

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

    printf("\nHot Food:\n");
    printf("  Avg delay: %.2f sec\n", total_delay_hot_food / num_delayed_hot_food);
    printf("  Max delay: %.2f sec\n", max_delay_hot_food);
    printf("  Time-average number in queue: %.2f\n", area_num_in_q_hot_food / sim_time);
    printf("  Max number in queue: %.0f\n", max_num_in_q_hot_food);

    printf("\nSpecialty Sandwich:\n");
    printf("  Avg delay: %.2f sec\n", total_delay_specialty / num_delayed_specialty);
    printf("  Max delay: %.2f sec\n", max_delay_specialty);
    printf("  Time-average number in queue: %.2f\n", area_num_in_q_specialty / sim_time);
    printf("  Max number in queue: %.0f\n", max_num_in_q_specialty);

    printf("\nCashier:\n");
    printf("  Avg delay: %.2f sec\n", total_delay_cashier / num_delayed_cashier);
    printf("  Max delay: %.2f sec\n", max_delay_cashier);
    printf("  Time-average number in queue: %.2f\n", area_num_in_q_cashier / sim_time);
    printf("  Max number in queue: %.0f\n", max_num_in_q_cashier);

    printf("\nCustomer Type Delays:\n");
    printf("  Hot Food: avg total delay = %.2f sec, max = %.2f sec\n",
        total_delay_type_hot / num_customers_hot, max_total_delay_type_hot);
    printf("  Specialty Sandwich: avg total delay = %.2f sec, max = %.2f sec\n",
        total_delay_type_specialty / num_customers_specialty, max_total_delay_type_specialty);
    printf("  Drinks only: avg total delay = %.2f sec, max = %.2f sec\n",
        total_delay_type_drink / num_customers_drink, max_total_delay_type_drink);

    float prob_hot = (float)num_customers_hot / (num_customers_hot + num_customers_specialty + num_customers_drink);
    float prob_specialty = (float)num_customers_specialty / (num_customers_hot + num_customers_specialty + num_customers_drink);
    float prob_drink = (float)num_customers_drink / (num_customers_hot + num_customers_specialty + num_customers_drink);
    float overall_avg_total_delay =
        prob_hot * (total_delay_type_hot / num_customers_hot) +
        prob_specialty * (total_delay_type_specialty / num_customers_specialty) +
        prob_drink * (total_delay_type_drink / num_customers_drink);
    printf("\nOverall average total delay (weighted): %.2f sec\n", overall_avg_total_delay);

    printf("Time-average number in system: %.2f\n", area_total_in_system / sim_time);
    printf("Max total number in system: %.0f\n", max_total_in_system);
}