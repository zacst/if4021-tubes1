#include "simlib.h"
#include <stdlib.h> // For exit()

#define EVENT_ARRIVAL_CAFETERIA 1
#define EVENT_DEPARTURE_CAFETERIA 2 // Not used, but good practice to keep
#define EVENT_ARRIVAL_HOT_FOOD 3
#define EVENT_DEPARTURE_HOT_FOOD 4
#define EVENT_ARRIVAL_SPECIALTY_SANDWICHES 5
#define EVENT_DEPARTURE_SPECIALTY_SANDWICHES 6
#define EVENT_ARRIVAL_DRINKS 7
#define EVENT_ARRIVAL_CASHIER 8
#define EVENT_DEPARTURE_CASHIER 9

#define LIST_QUEUE_HOT_FOOD 1
#define LIST_SERVER_HOT_FOOD 2
#define LIST_QUEUE_SPECIALTY_SANDWICHES 3
#define LIST_SERVER_SPECIALTY_SANDWICHES 4
#define LIST_QUEUE_CASHIER_BASE 5 // Queues for cashiers 0, 1, 2 will be 5, 6, 7
#define LIST_CASHIER_BASE 10      // Servers for cashiers 0, 1, 2 will be 10, 11, 12

#define STREAM_INTERARRIVAL 1
#define STREAM_GROUP_SIZE 2
#define STREAM_ROUTE 3
#define STREAM_HOT_FOOD_ST 4
#define STREAM_SPECIALTY_SANDWICHES_ST 5
#define STREAM_DRINKS_ST 6
#define STREAM_HOT_FOOD_ACT 7
#define STREAM_SPECIALTY_SANDWICHES_ACT 8
#define STREAM_DRINKS_ACT 9

#define CUST_HOT_FOOD 1
#define CUST_SPECIALTY 2
#define CUST_DRINKS_ONLY 3

// Model parameters
int min_hot_food_st, max_hot_food_st, min_hot_food_act, max_hot_food_act;
int min_specialty_sandwiches_st, max_specialty_sandwiches_st;
int min_specialty_sandwiches_act, max_specialty_sandwiches_act;
int min_drinks_st, max_drinks_st, min_drinks_act, max_drinks_act;
float mean_interarrival;

int num_hot_food_worker, num_specialty_sandwiches_worker, num_cashier;

double prob_distrib_num_customers[5];
double prob_distrib_routes[4];

// Statistics variables
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

void init_model(void) {
    int input_result;

    printf("Number of Hot Food Workers: ");
    input_result = scanf("%d", &num_hot_food_worker);
    if (input_result != 1) {
        printf("Invalid input\n");
        exit(1);
    }

    printf("Number of Specialty Sandwiches Workers: ");
    input_result = scanf("%d", &num_specialty_sandwiches_worker);
    if (input_result != 1) {
        printf("Invalid input\n");
        exit(1);
    }

    printf("Number of Cashiers: ");
    input_result = scanf("%d", &num_cashier);
    if (input_result != 1) {
        printf("Invalid input\n");
        exit(1);
    }

    if (num_cashier < 2 || num_cashier > 3) {
        printf("Error: Number of cashiers must be 2 or 3\n");
        exit(1);
    }
    if (num_hot_food_worker < 1 || num_specialty_sandwiches_worker < 1) {
        printf("Error: At least 1 worker required at each food station\n");
        exit(1);
    }

    // Base service times
    min_hot_food_st = 50;
    max_hot_food_st = 120;
    min_hot_food_act = 20;
    max_hot_food_act = 40;

    min_specialty_sandwiches_st = 60;
    max_specialty_sandwiches_st = 180;
    min_specialty_sandwiches_act = 5;
    max_specialty_sandwiches_act = 15;

    min_drinks_st = 5;
    max_drinks_st = 20;
    min_drinks_act = 5;
    max_drinks_act = 10;

    mean_interarrival = 30;

    // *** BUG FIX 2.1: Correctly adjust service times based on number of workers ***
    // Scenario (a)(ii): 2 workers at hot-food station cuts service time in half
    if (num_hot_food_worker == 2) {
        min_hot_food_st = 25;
        max_hot_food_st = 60;
    }
    // Scenario (a)(iii): 2 workers at specialty-sandwich station cuts service time in half
    if (num_specialty_sandwiches_worker == 2) {
        min_specialty_sandwiches_st = 30;
        max_specialty_sandwiches_st = 90;
    }

    prob_distrib_num_customers[1] = 0.5;
    prob_distrib_num_customers[2] = 0.8;
    prob_distrib_num_customers[3] = 0.9;
    prob_distrib_num_customers[4] = 1.0;

    prob_distrib_routes[1] = 0.80;
    prob_distrib_routes[2] = 0.95;
    prob_distrib_routes[3] = 1.00;

    event_schedule(expon(mean_interarrival, STREAM_INTERARRIVAL), EVENT_ARRIVAL_CAFETERIA);
}

void arrive_cafeteria(void) {
    int batch_size = random_integer(prob_distrib_num_customers, STREAM_GROUP_SIZE);

    for (int i = 0; i < batch_size; i++) {
        // transfer[1] = arrival time (set automatically by simlib)
        // transfer[2] = accumulated future cashier time (ACT)
        // transfer[3] = cashier index
        // transfer[4] = customer type
        // transfer[5] = accumulated delay time
        transfer[2] = 0.0;
        transfer[5] = 0.0;

        int route = random_integer(prob_distrib_routes, STREAM_ROUTE);
        int cust_type;
        int next_event;

        if (route == 1) {
            cust_type = CUST_HOT_FOOD;
            next_event = EVENT_ARRIVAL_HOT_FOOD;
        } else if (route == 2) {
            cust_type = CUST_SPECIALTY;
            next_event = EVENT_ARRIVAL_SPECIALTY_SANDWICHES;
        } else {
            cust_type = CUST_DRINKS_ONLY;
            next_event = EVENT_ARRIVAL_DRINKS;
        }

        transfer[4] = cust_type;
        event_schedule(sim_time, next_event);
    }

    event_schedule(sim_time + expon(mean_interarrival, STREAM_INTERARRIVAL), EVENT_ARRIVAL_CAFETERIA);
}

void arrive_hot_food(void) {
    // Problem states "customers are still served one at a time"
    if (list_size[LIST_SERVER_HOT_FOOD] == 1) {
        transfer[1] = sim_time; // Mark arrival time to the queue
        list_file(LAST, LIST_QUEUE_HOT_FOOD);
    } else {
        list_file(FIRST, LIST_SERVER_HOT_FOOD);
        // *** BUG FIX 2.2: Removed division by num_hot_food_worker ***
        double service_time = uniform(min_hot_food_st, max_hot_food_st, STREAM_HOT_FOOD_ST);
        event_schedule(sim_time + service_time, EVENT_DEPARTURE_HOT_FOOD);
    }
}

// *** BUG FIX 1.1: Restructured departure logic to prevent data loss ***
void depart_hot_food(void) {
    // 1. Handle the customer currently in transfer (the one departing the server).
    transfer[2] += uniform(min_hot_food_act, max_hot_food_act, STREAM_HOT_FOOD_ACT);
    event_schedule(sim_time, EVENT_ARRIVAL_DRINKS); // Send them to the next station

    // 2. Free the server.
    list_remove(FIRST, LIST_SERVER_HOT_FOOD);

    // 3. Handle the queue for the now-free server.
    if (list_size[LIST_QUEUE_HOT_FOOD] > 0) {
        list_remove(FIRST, LIST_QUEUE_HOT_FOOD); // Get next customer (their data is now in transfer).

        // Calculate and record their delay.
        float delay = sim_time - transfer[1];
        total_delay_hot_food += delay;
        num_delayed_hot_food++;
        if (delay > max_delay_hot_food) max_delay_hot_food = delay;
        transfer[5] += delay; // Add to this customer's personal total delay.

        // Start their service.
        list_file(FIRST, LIST_SERVER_HOT_FOOD);
        // *** BUG FIX 2.2: Removed division by num_hot_food_worker ***
        double service_time = uniform(min_hot_food_st, max_hot_food_st, STREAM_HOT_FOOD_ST);
        event_schedule(sim_time + service_time, EVENT_DEPARTURE_HOT_FOOD);
    }
}

void arrive_specialty_sandwiches(void) {
    // Problem states "service is still one at a time"
    if (list_size[LIST_SERVER_SPECIALTY_SANDWICHES] == 1) {
        transfer[1] = sim_time; // Mark arrival time to the queue
        list_file(LAST, LIST_QUEUE_SPECIALTY_SANDWICHES);
    } else {
        list_file(FIRST, LIST_SERVER_SPECIALTY_SANDWICHES);
        // *** BUG FIX 2.2: Removed division by num_specialty_sandwiches_worker ***
        double service_time = uniform(min_specialty_sandwiches_st, max_specialty_sandwiches_st, STREAM_SPECIALTY_SANDWICHES_ST);
        event_schedule(sim_time + service_time, EVENT_DEPARTURE_SPECIALTY_SANDWICHES);
    }
}

// *** BUG FIX 1.2: Restructured departure logic to prevent data loss ***
void depart_specialty_sandwiches(void) {
    // 1. Handle the customer currently in transfer (the one departing the server).
    transfer[2] += uniform(min_specialty_sandwiches_act, max_specialty_sandwiches_act, STREAM_SPECIALTY_SANDWICHES_ACT);
    event_schedule(sim_time, EVENT_ARRIVAL_DRINKS); // Send them to the next station

    // 2. Free the server.
    list_remove(FIRST, LIST_SERVER_SPECIALTY_SANDWICHES);

    // 3. Handle the queue for the now-free server.
    if (list_size[LIST_QUEUE_SPECIALTY_SANDWICHES] > 0) {
        list_remove(FIRST, LIST_QUEUE_SPECIALTY_SANDWICHES); // Get next customer.

        // Calculate and record their delay.
        float delay = sim_time - transfer[1];
        total_delay_specialty += delay;
        num_delayed_specialty++;
        if (delay > max_delay_specialty) max_delay_specialty = delay;
        transfer[5] += delay; // Add to this customer's personal total delay.

        // Start their service.
        list_file(FIRST, LIST_SERVER_SPECIALTY_SANDWICHES);
        // *** BUG FIX 2.2: Removed division by num_specialty_sandwiches_worker ***
        double service_time = uniform(min_specialty_sandwiches_st, max_specialty_sandwiches_st, STREAM_SPECIALTY_SANDWICHES_ST);
        event_schedule(sim_time + service_time, EVENT_DEPARTURE_SPECIALTY_SANDWICHES);
    }
}

void arrive_drinks(void) {
    // Add drinks ACT.
    transfer[2] += uniform(min_drinks_act, max_drinks_act, STREAM_DRINKS_ACT);
    // Schedule arrival at cashier after drinks ST.
    event_schedule(sim_time + uniform(min_drinks_st, max_drinks_st, STREAM_DRINKS_ST), EVENT_ARRIVAL_CASHIER);
}

void arrive_cashier(void) {
    // Try to find an idle cashier.
    for (int i = 0; i < num_cashier; ++i) {
        if (list_size[LIST_CASHIER_BASE + i] == 0) {
            transfer[3] = (double)i; // Store which cashier is serving this customer
            list_file(FIRST, LIST_CASHIER_BASE + i);
            event_schedule(sim_time + transfer[2], EVENT_DEPARTURE_CASHIER);
            return;
        }
    }

    // All cashiers are busy, so find the shortest queue.
    int shortest_q_idx = 0;
    int min_q_len = list_size[LIST_QUEUE_CASHIER_BASE + 0];
    for (int i = 1; i < num_cashier; ++i) {
        if (list_size[LIST_QUEUE_CASHIER_BASE + i] < min_q_len) {
            min_q_len = list_size[LIST_QUEUE_CASHIER_BASE + i];
            shortest_q_idx = i;
        }
    }

    transfer[1] = sim_time; // Mark arrival time to the queue
    list_file(LAST, LIST_QUEUE_CASHIER_BASE + shortest_q_idx);
}

void depart_cashier(void) {
    int cust_type = (int)transfer[4];
    float total_cust_delay = transfer[5];

    // Record final statistics by customer type.
    if (cust_type == CUST_HOT_FOOD) {
        total_delay_type_hot += total_cust_delay;
        if (total_cust_delay > max_total_delay_type_hot)
            max_total_delay_type_hot = total_cust_delay;
        num_customers_hot++;
    } else if (cust_type == CUST_SPECIALTY) {
        total_delay_type_specialty += total_cust_delay;
        if (total_cust_delay > max_total_delay_type_specialty)
            max_total_delay_type_specialty = total_cust_delay;
        num_customers_specialty++;
    } else {
        total_delay_type_drink += total_cust_delay;
        if (total_cust_delay > max_total_delay_type_drink)
            max_total_delay_type_drink = total_cust_delay;
        num_customers_drink++;
    }

    int cashier_idx = (int)transfer[3];
    list_remove(FIRST, LIST_CASHIER_BASE + cashier_idx);

    // Check this cashier's queue for the next customer.
    if (list_size[LIST_QUEUE_CASHIER_BASE + cashier_idx] > 0) {
        list_remove(FIRST, LIST_QUEUE_CASHIER_BASE + cashier_idx);

        // Calculate and record queue delay.
        float delay = sim_time - transfer[1];
        total_delay_cashier += delay;
        num_delayed_cashier++;
        if (delay > max_delay_cashier)
            max_delay_cashier = delay;
        transfer[5] += delay;

        // Start service for the new customer.
        transfer[3] = (double)cashier_idx;
        list_file(FIRST, LIST_CASHIER_BASE + cashier_idx);
        event_schedule(sim_time + transfer[2], EVENT_DEPARTURE_CASHIER);
    }
}

int main() {
    maxatr = 6;
    maxlist = 30; // Increased maxlist to accommodate more cashier queues if needed

    init_simlib();
    init_model();

    printf("\nSimulation running...\n");

    while (sim_time <= 5400.0) { // Run for 90 minutes (5400 seconds)
        timing();

        // Update time-average statistics
        float time_since_last_event = sim_time - time_last_event;
        time_last_event = sim_time;

        area_num_in_q_hot_food += list_size[LIST_QUEUE_HOT_FOOD] * time_since_last_event;
        area_num_in_q_specialty += list_size[LIST_QUEUE_SPECIALTY_SANDWICHES] * time_since_last_event;

        int total_cashier_queue = 0;
        for (int i = 0; i < num_cashier; i++)
            total_cashier_queue += list_size[LIST_QUEUE_CASHIER_BASE + i];
        area_num_in_q_cashier += total_cashier_queue * time_since_last_event;

        // Update max queue length statistics
        if (list_size[LIST_QUEUE_HOT_FOOD] > max_num_in_q_hot_food)
            max_num_in_q_hot_food = list_size[LIST_QUEUE_HOT_FOOD];
        if (list_size[LIST_QUEUE_SPECIALTY_SANDWICHES] > max_num_in_q_specialty)
            max_num_in_q_specialty = list_size[LIST_QUEUE_SPECIALTY_SANDWICHES];
        if (total_cashier_queue > max_num_in_q_cashier)
            max_num_in_q_cashier = total_cashier_queue;
        
        // Update system size statistics
        int total_in_system = list_size[LIST_QUEUE_HOT_FOOD] + list_size[LIST_SERVER_HOT_FOOD] +
                              list_size[LIST_QUEUE_SPECIALTY_SANDWICHES] + list_size[LIST_SERVER_SPECIALTY_SANDWICHES] +
                              total_cashier_queue;
        for (int i = 0; i < num_cashier; i++)
            total_in_system += list_size[LIST_CASHIER_BASE + i];

        area_total_in_system += total_in_system * time_since_last_event;
        if (total_in_system > max_total_in_system)
            max_total_in_system = total_in_system;


        switch (next_event_type) {
            case EVENT_ARRIVAL_CAFETERIA:
                arrive_cafeteria();
                break;
            case EVENT_ARRIVAL_HOT_FOOD:
                arrive_hot_food();
                break;
            case EVENT_DEPARTURE_HOT_FOOD:
                depart_hot_food();
                break;
            case EVENT_ARRIVAL_SPECIALTY_SANDWICHES:
                arrive_specialty_sandwiches();
                break;
            case EVENT_DEPARTURE_SPECIALTY_SANDWICHES:
                depart_specialty_sandwiches();
                break;
            case EVENT_ARRIVAL_DRINKS:
                arrive_drinks();
                break;
            case EVENT_ARRIVAL_CASHIER:
                arrive_cashier();
                break;
            case EVENT_DEPARTURE_CASHIER:
                depart_cashier();
                break;
        }
    }

    printf("\n=== SIMULATION RESULTS ===\n");
    printf("\nConfiguration: %d Hot Food, %d Specialty, %d Cashiers\n",
           num_hot_food_worker, num_specialty_sandwiches_worker, num_cashier);

    printf("\nHot Food Station:\n");
    printf("  Avg delay in queue: %.2f sec\n", (num_delayed_hot_food > 0) ? total_delay_hot_food / num_delayed_hot_food : 0.0);
    printf("  Max delay in queue: %.2f sec\n", max_delay_hot_food);
    printf("  Time-avg number in queue: %.2f\n", area_num_in_q_hot_food / sim_time);
    printf("  Max number in queue: %.0f\n", max_num_in_q_hot_food);

    printf("\nSpecialty Sandwich Station:\n");
    printf("  Avg delay in queue: %.2f sec\n", (num_delayed_specialty > 0) ? total_delay_specialty / num_delayed_specialty : 0.0);
    printf("  Max delay in queue: %.2f sec\n", max_delay_specialty);
    printf("  Time-avg number in queue: %.2f\n", area_num_in_q_specialty / sim_time);
    printf("  Max number in queue: %.0f\n", max_num_in_q_specialty);

    printf("\nCashier Stations (Combined):\n");
    printf("  Avg delay in queues: %.2f sec\n", (num_delayed_cashier > 0) ? total_delay_cashier / num_delayed_cashier : 0.0);
    printf("  Max delay in queues: %.2f sec\n", max_delay_cashier);
    printf("  Time-avg number in queues: %.2f\n", area_num_in_q_cashier / sim_time);
    printf("  Max number in queues: %.0f\n", max_num_in_q_cashier);

    printf("\nCustomer-Type Total Delay:\n");
    printf("  Hot Food: avg = %.2f sec, max = %.2f sec (n=%d)\n",
           (num_customers_hot > 0) ? total_delay_type_hot / num_customers_hot : 0.0,
           max_total_delay_type_hot, num_customers_hot);
    printf("  Specialty: avg = %.2f sec, max = %.2f sec (n=%d)\n",
           (num_customers_specialty > 0) ? total_delay_type_specialty / num_customers_specialty : 0.0,
           max_total_delay_type_specialty, num_customers_specialty);
    printf("  Drinks-Only: avg = %.2f sec, max = %.2f sec (n=%d)\n",
           (num_customers_drink > 0) ? total_delay_type_drink / num_customers_drink : 0.0,
           max_total_delay_type_drink, num_customers_drink);

    int total_customers_served = num_customers_hot + num_customers_specialty + num_customers_drink;
    float overall_avg_total_delay = (total_customers_served > 0) ?
        (total_delay_type_hot + total_delay_type_specialty + total_delay_type_drink) / total_customers_served : 0.0;

    printf("\nOverall Performance:\n");
    printf("  Overall avg total delay: %.2f sec\n", overall_avg_total_delay);
    printf("  Time-avg in system: %.2f\n", area_total_in_system / sim_time);
    printf("  Max in system: %.0f\n", max_total_in_system);
    printf("  Total customers served: %d\n", total_customers_served);

    return 0;
}