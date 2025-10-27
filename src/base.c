#include "simlib.h"              /* Required for use of simlib.c */

#define EVENT_ARRIVAL_CAFETERIA              1 /* Event type for arrival at cafeteria */
#define EVENT_DEPARTURE_CAFETERIA            2 /* Event type for arrival */
#define EVENT_ARRIVAL_HOT_FOOD               3 /* Event type for arrival */
#define EVENT_DEPARTURE_HOT_FOOD             4 /* Event type for arrival */
#define EVENT_ARRIVAL_SPECIALTY_SANDWICHES   5 /* Event type for arrival */
#define EVENT_DEPARTURE_SPECIALTY_SANDWICHES 6 /* Event type for arrival */
#define EVENT_ARRIVAL_DRINKS                 7 /* Event type for arrival at drink station */
#define EVENT_ARRIVAL_CASHIER                8 /* Event type for arrival at drink station */
#define EVENT_DEPARTURE_CASHIER              9 /* Event type for arrival at drink station */

/* Generate space for maximum number of use case */
#define LIST_QUEUE_CAFETERIA 1 /* 1 cafeteria queue */
#define LIST_QUEUE_HOT_FOOD 2 /* 2 hot food queue */
#define LIST_WORKER_HOT_FOOD 4 /* 2 hot food worker */
#define LIST_QUEUE_SPECIALTY_SANDWICHES 6 /* 2 specialty sandwiches queue */
#define LIST_WORKER_SPECIALTY_SANDWICHES 8 /* 2 specialty sandwiches worker */
#define LIST_QUEUE_CASHIER 10 /* 3 cashier queue */
#define LIST_CASHIER 13 /* 3 cashier */

#define STREAM_INTERARRIVAL 1                   /* Random-number stream for interarrivals */
#define STREAM_GROUP_SIZE 2                     /* Random-number stream for group sizes */
#define STREAM_ROUTE 3                          /* Random-number stream for route choice */
#define STREAM_ST_HOT_FOOD 4                    /* Random-number stream for hot food service times */
#define STREAM_ST_SPECIALTY 5                   /* Random-number stream for specialty sandwiches service times */
#define STREAM_ST_DRINKS 6                      /* Random-number stream for drinks service times */
#define STREAM_ACT_HOT_FOOD 7                   /* Random-number stream for hot food accumulated cashier times */
#define STREAM_ACT_SPECIALTY 8                  /* Random-number stream for specialty sandwiches accumulated cashier times */
#define STREAM_ACT_DRINKS 9                     /* Random-number stream for drinks accumulated cashier times */

/* Customer type identifiers */
#define CUST_HOT_FOOD        1
#define CUST_SPECIALTY       2
#define CUST_DRINKS_ONLY     3

/* Declare non-simlib global variables. */
int min_hot_food_st, max_hot_food_st, min_hot_food_act, max_hot_food_act;
int min_specialty_sandwiches_st, max_specialty_sandwiches_st, min_specialty_sandwiches_act, max_specialty_sandwiches_act;
int min_drinks_st, max_drinks_st, min_drinks_act, max_drinks_act;
float mean_interarrival;
int num_hot_food_worker, num_specialty_sandwiches_worker, num_cashier;
double prob_distrib_num_customers[5];
double prob_distrib_routes[4];

/* Performance statistics */
float total_delay_hot_food = 0.0, total_delay_specialty = 0.0, total_delay_cashier = 0.0;
float num_delayed_hot_food = 0.0, num_delayed_specialty = 0.0, num_delayed_cashier = 0.0;
float max_delay_hot_food = 0.0, max_delay_specialty = 0.0, max_delay_cashier = 0.0;
float area_num_in_q_hot_food = 0.0, area_num_in_q_specialty = 0.0, area_num_in_q_cashier = 0.0;
float max_num_in_q_hot_food = 0.0, max_num_in_q_specialty = 0.0, max_num_in_q_cashier = 0.0;
float total_delay_type_hot = 0.0, total_delay_type_specialty = 0.0, total_delay_type_drink = 0.0;
float max_total_delay_type_hot = 0.0, max_total_delay_type_specialty = 0.0, max_total_delay_type_drink = 0.0;
int num_customers_hot = 0, num_customers_specialty = 0, num_customers_drink = 0;
float area_total_in_system = 0.0;
float max_total_in_system = 0.0;
float time_last_event = 0.0;

/* Helper Functions */

/* Handle the route random generator */
int map_route_int_to_event_type(int route_int) {
    switch (route_int) {
        case 1:
            transfer[4] = CUST_HOT_FOOD;
            num_customers_hot++; /* FIX #2: Count customer immediately */
            return EVENT_ARRIVAL_HOT_FOOD;
        case 2:
            transfer[4] = CUST_SPECIALTY;
            num_customers_specialty++; /* FIX #2: Count customer immediately */
            return EVENT_ARRIVAL_SPECIALTY_SANDWICHES;
        case 3:
            transfer[4] = CUST_DRINKS_ONLY;
            num_customers_drink++; /* FIX #2: Count customer immediately */
            return EVENT_ARRIVAL_DRINKS;
    }
    return 0;
}

void init_model(void) {
    printf("Number of Hot Food Workers: ");
    scanf("%d", &num_hot_food_worker);
    printf("Number of Specialty Sandwiches Workers: ");
    scanf("%d", &num_specialty_sandwiches_worker);
    printf("Number of Cashiers: ");
    scanf("%d", &num_cashier);

    min_hot_food_st = 50; max_hot_food_st = 120;
    min_hot_food_act = 20; max_hot_food_act = 40;
    min_specialty_sandwiches_st = 60; max_specialty_sandwiches_st = 180;
    min_specialty_sandwiches_act = 5; max_specialty_sandwiches_act = 15;
    min_drinks_st = 5; max_drinks_st = 20;
    min_drinks_act = 5; max_drinks_act = 10;
    mean_interarrival = 30;

    prob_distrib_num_customers[1] = 0.5;
    prob_distrib_num_customers[2] = 0.8;
    prob_distrib_num_customers[3] = 0.9;
    prob_distrib_num_customers[4] = 1;
    prob_distrib_routes[1] = 0.8;
    prob_distrib_routes[2] = 0.95;
    prob_distrib_routes[3] = 1;

    event_schedule(sim_time + expon(mean_interarrival, STREAM_INTERARRIVAL), EVENT_ARRIVAL_CAFETERIA);
}

void arrive_cafeteria(void) {
    int batch_size = random_integer(prob_distrib_num_customers, STREAM_GROUP_SIZE);
    for (int i = 0; i < batch_size; i++) {
        int route = random_integer(prob_distrib_routes, STREAM_ROUTE);
        int next_event = map_route_int_to_event_type(route);
        transfer[2] = 0.0;
        event_schedule(sim_time, next_event);
    }
    event_schedule(sim_time + expon(mean_interarrival, STREAM_INTERARRIVAL), EVENT_ARRIVAL_CAFETERIA);
}

void arrive_hot_food(void) {
    int chosen_worker = 0;
    int least_queue_length = list_size[LIST_QUEUE_HOT_FOOD];

    for (int i = 0; i < num_hot_food_worker; i++) {
        if (list_size[LIST_WORKER_HOT_FOOD + i] == 0) {
            transfer[1] = sim_time;
            transfer[3] = (double)i;
            transfer[4] = CUST_HOT_FOOD;
            list_file(FIRST, LIST_WORKER_HOT_FOOD + i);
            double service_time = uniform(min_hot_food_st, max_hot_food_st, STREAM_ST_HOT_FOOD);
            event_schedule(sim_time + service_time, EVENT_DEPARTURE_HOT_FOOD);
            return;
        }
        if (list_size[LIST_QUEUE_HOT_FOOD + i] < least_queue_length) {
            least_queue_length = list_size[LIST_QUEUE_HOT_FOOD + i];
            chosen_worker = i;
        }
    }
    transfer[1] = sim_time;
    transfer[4] = CUST_HOT_FOOD;
    list_file(LAST, LIST_QUEUE_HOT_FOOD + chosen_worker);
}

void depart_hot_food(void) {
    int worker_idx = (int) transfer[3];
    list_remove(FIRST, LIST_WORKER_HOT_FOOD + worker_idx);

    if (list_size[LIST_QUEUE_HOT_FOOD + worker_idx] > 0) {
        list_remove(FIRST, LIST_QUEUE_HOT_FOOD + worker_idx);
        float delay = sim_time - transfer[1];
        total_delay_hot_food += delay;
        num_delayed_hot_food++;
        if (delay > max_delay_hot_food) max_delay_hot_food = delay;
        total_delay_type_hot += delay;
        if (delay > max_total_delay_type_hot) max_total_delay_type_hot = delay;

        if (list_size[LIST_QUEUE_HOT_FOOD + worker_idx] > max_num_in_q_hot_food)
            max_num_in_q_hot_food = list_size[LIST_QUEUE_HOT_FOOD + worker_idx];

        transfer[3] = (double) worker_idx;
        list_file(FIRST, LIST_WORKER_HOT_FOOD + worker_idx);
        double service_time = uniform(min_hot_food_st, max_hot_food_st, STREAM_ST_HOT_FOOD);
        event_schedule(sim_time + service_time, EVENT_DEPARTURE_HOT_FOOD);
    }
    transfer[2] += uniform(min_hot_food_act, max_hot_food_act, STREAM_ACT_HOT_FOOD);
    event_schedule(sim_time, EVENT_ARRIVAL_DRINKS);
}

void arrive_specialty_sandwiches(void) {
    int chosen_worker = 0;
    int least_queue_length = list_size[LIST_QUEUE_SPECIALTY_SANDWICHES];

    for (int i = 0; i < num_specialty_sandwiches_worker; i++) {
        if (list_size[LIST_WORKER_SPECIALTY_SANDWICHES + i] == 0) {
            transfer[1] = sim_time;
            transfer[3] = (double)i;
            transfer[4] = CUST_SPECIALTY;
            list_file(FIRST, LIST_WORKER_SPECIALTY_SANDWICHES + i);
            double service_time = uniform(min_specialty_sandwiches_st, max_specialty_sandwiches_st, STREAM_ST_SPECIALTY);
            event_schedule(sim_time + service_time, EVENT_DEPARTURE_SPECIALTY_SANDWICHES);
            return;
        }
        if (list_size[LIST_QUEUE_SPECIALTY_SANDWICHES + i] < least_queue_length) {
            least_queue_length = list_size[LIST_QUEUE_SPECIALTY_SANDWICHES + i];
            chosen_worker = i;
        }
    }
    transfer[1] = sim_time;
    transfer[4] = CUST_SPECIALTY;
    list_file(LAST, LIST_QUEUE_SPECIALTY_SANDWICHES + chosen_worker);
}

void depart_specialty_sandwiches(void) {
    int worker_idx = (int) transfer[3];
    list_remove(FIRST, LIST_WORKER_SPECIALTY_SANDWICHES + worker_idx);

    if (list_size[LIST_QUEUE_SPECIALTY_SANDWICHES + worker_idx] > 0) {
        list_remove(FIRST, LIST_QUEUE_SPECIALTY_SANDWICHES + worker_idx);
        float delay = sim_time - transfer[1];
        total_delay_specialty += delay;
        num_delayed_specialty++;
        if (delay > max_delay_specialty) max_delay_specialty = delay;
        total_delay_type_specialty += delay;
        if (delay > max_total_delay_type_specialty) max_total_delay_type_specialty = delay;

        if (list_size[LIST_QUEUE_SPECIALTY_SANDWICHES + worker_idx] > max_num_in_q_specialty)
            max_num_in_q_specialty = list_size[LIST_QUEUE_SPECIALTY_SANDWICHES + worker_idx];

        transfer[3] = (double) worker_idx;
        list_file(FIRST, LIST_WORKER_SPECIALTY_SANDWICHES + worker_idx);
        double service_time = uniform(min_specialty_sandwiches_st, max_specialty_sandwiches_st, STREAM_ST_SPECIALTY);
        event_schedule(sim_time + service_time, EVENT_DEPARTURE_SPECIALTY_SANDWICHES);
    }
    transfer[2] += uniform(min_specialty_sandwiches_act, max_specialty_sandwiches_act, STREAM_ACT_SPECIALTY);
    event_schedule(sim_time, EVENT_ARRIVAL_DRINKS);
}

void arrive_drinks(void) {
    transfer[2] += uniform(min_drinks_act, max_drinks_act, STREAM_ACT_DRINKS);
    event_schedule(sim_time + uniform(min_drinks_st, max_drinks_st, STREAM_ST_DRINKS), EVENT_ARRIVAL_CASHIER);
}

void arrive_cashier(void) {
    /* FIX #1: Always record arrival time to the cashier station for THIS customer. */
    transfer[1] = sim_time;

    int chosen_cashier = 0;
    int least_queue_length = list_size[LIST_QUEUE_CASHIER + 0];
    for (int i = 1; i < num_cashier; ++i) {
        if (list_size[LIST_QUEUE_CASHIER + i] < least_queue_length) {
            least_queue_length = list_size[LIST_QUEUE_CASHIER + i];
            chosen_cashier = i;
        }
    }

    if (list_size[LIST_CASHIER + chosen_cashier] == 0) {
        list_file(FIRST, LIST_CASHIER + chosen_cashier);
        transfer[3] = (double)chosen_cashier;
        double service_time = transfer[2];
        event_schedule(sim_time + service_time, EVENT_DEPARTURE_CASHIER);
    } else {
        /* transfer[1] was already set above */
        list_file(LAST, LIST_QUEUE_CASHIER + chosen_cashier);
    }
}

void depart_cashier(void) {
    int cashier_idx = (int) transfer[3];

    /* FIX #1: Compute delay for THE CUSTOMER WHO IS LEAVING, not the next one. */
    float delay = sim_time - transfer[1];
    total_delay_cashier += delay;
    num_delayed_cashier++; /* Now we count EVERY customer, even those with 0 delay */
    if (delay > max_delay_cashier) max_delay_cashier = delay;

    int cust_type = (int) transfer[4];
    if (cust_type == CUST_HOT_FOOD) {
        total_delay_type_hot += delay;
        if (delay > max_total_delay_type_hot) max_total_delay_type_hot = delay;
    } else if (cust_type == CUST_SPECIALTY) {
        total_delay_type_specialty += delay;
        if (delay > max_total_delay_type_specialty) max_total_delay_type_specialty = delay;
    } else if (cust_type == CUST_DRINKS_ONLY) {
        total_delay_type_drink += delay;
        if (delay > max_total_delay_type_drink) max_total_delay_type_drink = delay;
    }
    
    list_remove(FIRST, LIST_CASHIER + cashier_idx);

    /* Now, check if anyone was waiting and start their service. */
    if (list_size[LIST_QUEUE_CASHIER + cashier_idx] > 0) {
        list_remove(FIRST, LIST_QUEUE_CASHIER + cashier_idx);
        transfer[3] = (double) cashier_idx;
        list_file(FIRST, LIST_CASHIER + cashier_idx);
        double service_time = transfer[2];
        event_schedule(sim_time + service_time, EVENT_DEPARTURE_CASHIER);
    }
}

int main() {
    maxatr = 5;
    maxlist = 30;
    init_simlib();
    init_model();

    while (sim_time <= 5400) {
        timing();
        float time_since_last_event = sim_time - time_last_event;
        time_last_event = sim_time;
        area_num_in_q_hot_food += list_size[LIST_QUEUE_HOT_FOOD] * time_since_last_event;
        area_num_in_q_specialty += list_size[LIST_QUEUE_SPECIALTY_SANDWICHES] * time_since_last_event;
        int total_cashier_queue = 0;
        for (int i = 0; i < num_cashier; i++)
            total_cashier_queue += list_size[LIST_QUEUE_CASHIER + i];
        area_num_in_q_cashier += total_cashier_queue * time_since_last_event;

        int total_in_system = 0;
        for (int i = 0; i < num_hot_food_worker; i++)
            total_in_system += list_size[LIST_WORKER_HOT_FOOD + i] + list_size[LIST_QUEUE_HOT_FOOD + i];
        for (int i = 0; i < num_specialty_sandwiches_worker; i++)
            total_in_system += list_size[LIST_WORKER_SPECIALTY_SANDWICHES + i] + list_size[LIST_QUEUE_SPECIALTY_SANDWICHES + i];
        for (int i = 0; i < num_cashier; i++)
            total_in_system += list_size[LIST_CASHIER + i] + list_size[LIST_QUEUE_CASHIER + i];
        area_total_in_system += total_in_system * time_since_last_event;
        if (total_in_system > max_total_in_system) max_total_in_system = total_in_system;

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
    printf("  Hot Food: avg total delay = %.2f sec, max = %.2f sec\n", total_delay_type_hot / num_customers_hot, max_total_delay_type_hot);
    printf("  Specialty Sandwich: avg total delay = %.2f sec, max = %.2f sec\n", total_delay_type_specialty / num_customers_specialty, max_total_delay_type_specialty);
    printf("  Drinks only: avg total delay = %.2f sec, max = %.2f sec\n", total_delay_type_drink / num_customers_drink, max_total_delay_type_drink);

    float prob_hot = (float)num_customers_hot / (num_customers_hot + num_customers_specialty + num_customers_drink);
    float prob_specialty = (float)num_customers_specialty / (num_customers_hot + num_customers_specialty + num_customers_drink);
    float prob_drink = (float)num_customers_drink / (num_customers_hot + num_customers_specialty + num_customers_drink);
    float overall_avg_total_delay = prob_hot * (total_delay_type_hot / num_customers_hot) + prob_specialty * (total_delay_type_specialty / num_customers_specialty) + prob_drink * (total_delay_type_drink / num_customers_drink);
    printf("\nOverall average total delay (weighted): %.2f sec\n", overall_avg_total_delay);

    printf("Time-average number in system: %.2f\n", area_total_in_system / sim_time);
    printf("Max total number in system: %.0f\n", max_total_in_system);

    return 0;
}