#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 20
//This is microseconds. Sensors have to be fast.

//WE start with sensor definitions. Not too harsh on reqs.
typedef struct {
    int sensor_id;
    char sensor_type[20];
    float value;
    char timestamp[30];
} sensor_log_t;

//We can actually define a linked list in this way. Simplicity.
typedef struct log_node {
    sensor_log_t data;
    struct log_node* next;
    struct log_node* prev;
} log_node_t;

//GLobal vars
log_node_t* head = NULL;
log_node_t* tail = NULL;
log_node_t* current_log = NULL;
int log_count = 0;
int log_counter = 1;

//Sync prerequsities. Pthread is surprisingly necessary due to mutex.
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
int live_stream_active = 0;
int system_running = 1;

void get_timestamp(char *buffer) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, 30, "%Y-%m-%d %H:%M:%S", t);
}

void add_sensor_log(const char* sensor_type, float value) {
    pthread_mutex_lock(&list_mutex);
    
    //Creating new node
    log_node_t* new_node = (log_node_t*)malloc(sizeof(log_node_t));
    new_node->data.sensor_id = log_counter++;
    strcpy(new_node->data.sensor_type, sensor_type);
    new_node->data.value = value;
    get_timestamp(new_node->data.timestamp);
    new_node->next = NULL;
    new_node->prev = tail;
    
    if (log_count >= MAX_BUFFER_SIZE) {
        log_node_t* old_head = head;
        head = head->next;
        if (head) head->prev = NULL;
        if (current_log == old_head) current_log = head;
        free(old_head);
        log_count--;
    }
    
    if (tail) tail->next = new_node;
    tail = new_node;
    if (!head) {
        head = new_node;
        current_log = new_node;
    }
    log_count++;
    pthread_mutex_unlock(&list_mutex);
}

void display_current_log() {
    pthread_mutex_lock(&list_mutex);
    if (current_log) {
        printf("\n[Log #%d] %s | Value: %.2f | Time: %s\n",
               current_log->data.sensor_id,
               current_log->data.sensor_type,
               current_log->data.value,
               current_log->data.timestamp);
    } else {
        printf("\nNo logs available.\n");
    }
    pthread_mutex_unlock(&list_mutex);
}

void navigate_next() {
    pthread_mutex_lock(&list_mutex);
    if (current_log && current_log->next) {
        current_log = current_log->next;
        pthread_mutex_unlock(&list_mutex);
        display_current_log();
    } else {
        pthread_mutex_unlock(&list_mutex);
        printf("\nAlready at the most recent log.\n");
    }
}

void navigate_previous() {
    pthread_mutex_lock(&list_mutex);
    if (current_log && current_log->prev) {
        current_log = current_log->prev;
        pthread_mutex_unlock(&list_mutex);
        display_current_log();
    } else {
        pthread_mutex_unlock(&list_mutex);
        printf("\nAlready at the oldest log.\n");
    }
}

void clear_all_logs() {
    pthread_mutex_lock(&list_mutex);
    log_node_t* current = head;
    while (current != NULL) {
        log_node_t* next = current->next;
        free(current);
        current = next;
    }
    head = tail = current_log = NULL;
    log_count = 0;
    log_counter = 1;
    pthread_mutex_unlock(&list_mutex);
    printf("\nAll logs cleared.\n");
}

void *live_stream_function(void *arg) {
    (void)arg;
    const char *sensors[] = {"Temperature", "Humidity", "Pressure", "Vibration"};
    while (live_stream_active) {
        float value = 20.0 + (rand() % 300) / 10.0;
        int sensor_index = rand() % 4;
        add_sensor_log(sensors[sensor_index], value);
        printf("[LIVE] New log added: %s = %.2f\n", sensors[sensor_index], value);
        sleep(2);
    }
    return NULL;
}

void start_live_stream() {
    if (!live_stream_active) {
        live_stream_active = 1;
        pthread_t thread;
        pthread_create(&thread, NULL, live_stream_function, NULL);
        pthread_detach(thread);
        printf("\nLive streaming started. Press 'z' to pause.\n");
    } else {
        printf("\nLive mode already active.\n");
    }
}

void stop_live_stream() {
    live_stream_active = 0;
    printf("\nLive streaming paused.\n");
}

int main() {
    srand(time(NULL));
    char command;
    
    printf("!!!!! IoT Gateway Log System !!!!!\n");
    printf("Commands:\n");
    printf("  n -Next log\n");
    printf("  p -Previous log\n");
    printf("  y -Start live streaming\n");
    printf("  z -Pause live streaming\n");
    printf("  c -Clear all logs\n");
    printf("  s -Save and exit\n");
    printf("==============================\n");
    
    add_sensor_log("Temperature", 25.5);
    add_sensor_log("Humidity", 60.2);
    add_sensor_log("Pressure", 1013.25);
    
    current_log = head;
    display_current_log();
    
    while (system_running) {
        printf("\nEnter command: ");
        scanf(" %c", &command);
        
        switch (command) {
            case 'n': navigate_next(); break;
            case 'p': navigate_previous(); break;
            case 'y': start_live_stream(); break;
            case 'z': stop_live_stream(); break;
            case 'c': clear_all_logs(); break;
            case 's': 
                printf("\nSaving session and exiting...\n");
                system_running = 0;
                break;
            default: printf("\nInvalid command.\n");
        }
    }
    
    clear_all_logs();
    printf("System terminated.\n");
    return 0;
}