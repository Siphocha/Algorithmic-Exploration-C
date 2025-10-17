#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_LOGS 20

typedef struct SensorLog {
    int id;
    char sensor_type[20];
    float value;
    char timestamp[30];
    struct SensorLog *prev;
    struct SensorLog *next;
} SensorLog;

typedef struct {
    SensorLog *head;
    SensorLog *tail;
    SensorLog *current;
    int count;
    pthread_mutex_t lock;
    int live_mode;
} LogSystem;

LogSystem log_system = {NULL, NULL, NULL, 0, PTHREAD_MUTEX_INITIALIZER, 0};
int log_counter = 1;

void get_timestamp(char *buffer) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, 30, "%Y-%m-%d %H:%M:%S", t);
}

void add_log(const char *sensor_type, float value) {
    pthread_mutex_lock(&log_system.lock);
    
    SensorLog *new_log = (SensorLog *)malloc(sizeof(SensorLog));
    new_log->id = log_counter++;
    strcpy(new_log->sensor_type, sensor_type);
    new_log->value = value;
    get_timestamp(new_log->timestamp);
    new_log->next = NULL;
    new_log->prev = log_system.tail;
    
    if (log_system.count >= MAX_LOGS) {
        SensorLog *old_head = log_system.head;
        log_system.head = old_head->next;
        if (log_system.head) {
            log_system.head->prev = NULL;
        }
        if (log_system.current == old_head) {
            log_system.current = log_system.head;
        }
        free(old_head);
        log_system.count--;
    }
    
    if (log_system.tail) {
        log_system.tail->next = new_log;
    }
    log_system.tail = new_log;
    
    if (!log_system.head) {
        log_system.head = new_log;
        log_system.current = new_log;
    }
    
    log_system.count++;
    pthread_mutex_unlock(&log_system.lock);
}

void display_current_log() {
    pthread_mutex_lock(&log_system.lock);
    if (log_system.current) {
        printf("\n[Log #%d] %s | Type: %s | Value: %.2f | Time: %s\n",
               log_system.current->id,
               log_system.current->sensor_type,
               log_system.current->sensor_type,
               log_system.current->value,
               log_system.current->timestamp);
    } else {
        printf("\nNo logs available.\n");
    }
    pthread_mutex_unlock(&log_system.lock);
}

void navigate_next() {
    pthread_mutex_lock(&log_system.lock);
    if (log_system.current && log_system.current->next) {
        log_system.current = log_system.current->next;
        pthread_mutex_unlock(&log_system.lock);
        display_current_log();
    } else {
        pthread_mutex_unlock(&log_system.lock);
        printf("\nAlready at the most recent log.\n");
    }
}

void navigate_previous() {
    pthread_mutex_lock(&log_system.lock);
    if (log_system.current && log_system.current->prev) {
        log_system.current = log_system.current->prev;
        pthread_mutex_unlock(&log_system.lock);
        display_current_log();
    } else {
        pthread_mutex_unlock(&log_system.lock);
        printf("\nAlready at the oldest log.\n");
    }
}

void clear_logs() {
    pthread_mutex_lock(&log_system.lock);
    SensorLog *temp;
    while (log_system.head) {
        temp = log_system.head;
        log_system.head = log_system.head->next;
        free(temp);
    }
    log_system.head = log_system.tail = log_system.current = NULL;
    log_system.count = 0;
    pthread_mutex_unlock(&log_system.lock);
    printf("\nAll logs cleared.\n");
}

void *live_stream_thread(void *arg) {
    (void)arg; // Suppress unused parameter warning
    const char *sensors[] = {"Temperature", "Humidity", "Pressure", "Vibration"};
    while (log_system.live_mode) {
        float value = 20.0 + (rand() % 300) / 10.0;
        int sensor_index = rand() % 4;
        add_log(sensors[sensor_index], value);
        printf("[LIVE] New log added: %s = %.2f\n", sensors[sensor_index], value);
        sleep(2);
    }
    return NULL;
}

void start_live_mode() {
    if (!log_system.live_mode) {
        log_system.live_mode = 1;
        pthread_t thread;
        pthread_create(&thread, NULL, live_stream_thread, NULL);
        pthread_detach(thread);
        printf("\nLive streaming started. Press 'z' to pause.\n");
    } else {
        printf("\nLive mode already active.\n");
    }
}

void stop_live_mode() {
    log_system.live_mode = 0;
    printf("\nLive streaming paused.\n");
}

int main() {
    srand(time(NULL));
    char command;
    
    printf("=== IoT Gateway Log System ===\n");
    printf("Commands:\n");
    printf("  n - Next log\n");
    printf("  p - Previous log\n");
    printf("  y - Start live streaming\n");
    printf("  z - Pause live streaming\n");
    printf("  c - Clear all logs\n");
    printf("  s - Save and exit\n");
    printf("==============================\n");
    
    add_log("Temperature", 25.5);
    add_log("Humidity", 60.2);
    add_log("Pressure", 1013.25);
    
    log_system.current = log_system.head;
    display_current_log();
    
    while (1) {
        printf("\nEnter command: ");
        scanf(" %c", &command);
        
        switch (command) {
            case 'n':
                navigate_next();
                break;
            case 'p':
                navigate_previous();
                break;
            case 'y':
                start_live_mode();
                break;
            case 'z':
                stop_live_mode();
                break;
            case 'c':
                clear_logs();
                break;
            case 's':
                printf("\nSaving session and exiting...\n");
                clear_logs();
                pthread_mutex_destroy(&log_system.lock);
                return 0;
            default:
                printf("\nInvalid command.\n");
        }
    }
    
    return 0;
}