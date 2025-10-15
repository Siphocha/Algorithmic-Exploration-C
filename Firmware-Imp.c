#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
//DID this as temporary solution for unistd.h error
#ifdef _WIN32
#include <io.h>
#define access _access

#else
#include <unistd.h>
#endif
//Again. This is temp resolution for unistd.h import error

#include <time.h>

#define MAX_BUFFER_SIZE 20
#define SENSOR_DATA_STREAM_DELAY 100000  
//This is microseconds. Sensors have to be fast.

//WE start with sensor definitions. Not too harsh on reqs.
typedef struct {
    int sensor_id;
    float temperature;
    float humidity;
    float pressure;
    float vibration;
    time_t timestamp;
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

//Sync prerequsities. Pthread is surprisingly necessary due to mutex.
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t live_stream_thread;
int live_stream_active = 0;
int system_running = 1;
int live_updates_paused = 0;

//ALL functions declared
void initialise_system();
void cleanup_system();
void add_sensor_log(int sensor_id, float temp, float hum, float press, float vib);
void display_current_log();
void navigate_next();
void navigate_previous();
void start_live_stream();
void stop_live_stream();
void* live_stream_function(void* arg);
void halt_live_updates();
void resume_live_updates();
void clear_all_logs();
void save_session_state();
void generate_sensor_data();

//Initialise system
void initialise_system() {
    pthread_mutex_lock(&list_mutex);
    clear_all_logs();
    
    //Testing data. Will probably remove soon...probably.
    for (int i = 0; i < 3; i++) {
        add_sensor_log(i+1, 20.0 + i, 45.0 + i, 1013.0 + i, 0.1 + i*0.1);
    }
    current_log = head;
    pthread_mutex_unlock(&list_mutex);
    printf("Starting at first log.\n");
    display_current_log();
}

//Cleaning up system resources
void cleanup_system() {
    stop_live_stream();
    clear_all_logs();
}

void add_sensor_log(int sensor_id, float temp, float hum, float press, float vib) {
    pthread_mutex_lock(&list_mutex);
    
    //Creating new node
    log_node_t* new_node = (log_node_t*)malloc(sizeof(log_node_t));
    if (!new_node) {
        printf("Memory allocation failed!\n");
        pthread_mutex_unlock(&list_mutex);
        return;
    }
    
    //Filling data with required new node attachements and defs.
    new_node->data.sensor_id = sensor_id;
    new_node->data.temperature = temp;
    new_node->data.humidity = hum;
    new_node->data.pressure = press;
    new_node->data.vibration = vib;
    new_node->data.timestamp = time(NULL);
    
    new_node->next = NULL;
    new_node->prev = NULL;
    
    //Handle incase empty
    if (head == NULL) {
        head = new_node;
        tail = new_node;
        current_log = new_node;
        log_count = 1;
    } else {
        new_node->prev = tail;
        tail->next = new_node;
        tail = new_node;
        log_count++;
        
        if (log_count > MAX_BUFFER_SIZE) {
            log_node_t* old_head = head;
            head = head->next;
            if (head) {
                head->prev = NULL;
            }
            
            //Log updates
            if (current_log == old_head) {
                current_log = head;
            }
            
            free(old_head);
            log_count--;
            printf("Buffer full - updates logs.\n");
        }
    }
    
    pthread_mutex_unlock(&list_mutex);
}

void display_current_log() {
    if (current_log == NULL) {
        printf("No logs available at all.\n");
        return;
    }
    
    sensor_log_t* log = &current_log->data;
    struct tm* timeinfo = localtime(&log->timestamp);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    printf("\n!!!!! Current Sensor Log !!!!!\n");
    printf("Time: %s\n", time_str);
    printf("Sensor ID: %d\n", log->sensor_id);
    printf("Temperature: %.2f°C\n", log->temperature);
    printf("Humidity: %.2f%%\n", log->humidity);
    printf("Pressure: %.2f hPa\n", log->pressure);
    printf("Vibration: %.2f m/s²\n", log->vibration);
    printf("Log position: %d/%d\n", 
           (current_log == head ? 1 : (current_log == tail ? log_count : -1)), 
           log_count);
    printf("Live: %s\n", live_stream_active ? (live_updates_paused ? "PAUSING" : "ACTIVE") : "INACTIVITY");
}

//Navigating to net log(newestt)
void navigate_next() {
    pthread_mutex_lock(&list_mutex);
    
    if (current_log == NULL) {
        printf("No logs available\n");
    } else if (current_log->next != NULL) {
        current_log = current_log->next;
        display_current_log();
    } else {
        printf("Already there\n");
    }
    
    pthread_mutex_unlock(&list_mutex);
}

void navigate_previous() {
    pthread_mutex_lock(&list_mutex);
    
    if (current_log == NULL) {
        printf("No logs available.\n");
    } else if (current_log->prev != NULL) {
        current_log = current_log->prev;
        display_current_log();
    } else {
        printf("Already there\n");
    }
    
    pthread_mutex_unlock(&list_mutex);
}

void start_live_stream() {
    if (live_stream_active) {
        printf("Live stream is active\n");
        return;
    }
    
    live_stream_active = 1;
    live_updates_paused = 0;
    
    if (pthread_create(&live_stream_thread, NULL, live_stream_function, NULL) != 0) {
        printf("live stream isn't working\n");
        live_stream_active = 0;
        return;
    }
    
    printf("Live stream started\n");
}

void stop_live_stream() {
    if (live_stream_active) {
        live_stream_active = 0;
        pthread_join(live_stream_thread, NULL);
        printf("Live stream has been stopped.\n");
    }
}

//Handling of the live stream thread itself.
void* live_stream_function(void* arg) {
    while (live_stream_active && system_running) {
        if (!live_updates_paused) {
            generate_sensor_data();
        }
        usleep(SENSOR_DATA_STREAM_DELAY);
    }
    return NULL;
}

//PAUSE live updates
void halt_live_updates() {
    if (live_stream_active) {
        live_updates_paused = 1;
        printf("Live updates temporarily halted.\n");
    } else {
        printf("Live stream is not active.\n");
    }
}

void resume_live_updates() {
    if (live_stream_active && live_updates_paused) {
        live_updates_paused = 0;
        printf("Live updates continued\n");
    } else if (!live_stream_active) {
        printf("Live stream isn't active\n");
    } else {
        printf("Live updates are Active\n");
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
    
    head = NULL;
    tail = NULL;
    current_log = NULL;
    log_count = 0;
    
    pthread_mutex_unlock(&list_mutex);
    
    printf("All logs cleared. MEMORY IS FREE\n");
}

//Saving simulated session state
void save_session_state() {
    pthread_mutex_lock(&list_mutex);
    
    printf("Saving session state...\n");
    // In a real implementation, this would save to persistent storage
    printf("Session state saved successfully.\n");
    
    pthread_mutex_unlock(&list_mutex);
}

//Generate simulated sensor data
void generate_sensor_data() {
    static int sensor_id = 1;
    float temp = 20.0 + (rand() % 150) / 10.0;  //20.0-35.0°C
    float hum = 40.0 + (rand() % 300) / 10.0;   //40.0-70.0%
    float press = 1010.0 + (rand() % 90) / 10.0; //1010.0-1019.0 hPa
    float vib = (rand() % 70) / 10.0;           //0.0-7.0 m/s²
    
    add_sensor_log(sensor_id, temp, hum, press, vib);
    
    // Rotate through sensor IDs 1-5
    sensor_id = (sensor_id % 5) + 1;
}

//Menu for helping
void print_help() {
    printf("\n!!!!! COMMANDS MENU !!!!!\n");
    printf("n - Most recent log\n");
    printf("p - Older log\n");
    printf("y - Live Stream Sensor Data\n");
    printf("z - Halt live updates\n");
    printf("r - Continue live updates\n");
    printf("s - Save Session and Terminate\n");
    printf("c - Clear Logs and Free Memory\n");
    printf("h - Show Menu\n");
    printf("q - Quit without saving\n");
}

int main() {
    char command;
    
    printf("!!!!! IoT Firmware !!!!!!\n");
    printf("Maximum buffer size: %d logs\n", MAX_BUFFER_SIZE);
    
    //Initialising system on startup
    initialise_system();
    print_help();
    
    // Main command loop
    while (system_running) {
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
                start_live_stream();
                break;
            case 'z':
                halt_live_updates();
                break;
            case 'r':
                resume_live_updates();
                break;
            case 's':
                save_session_state();
                system_running = 0;
                break;
            case 'c':
                clear_all_logs();
                break;
            case 'h':
                print_help();
                break;
            case 'q':
                system_running = 0;
                printf("Exiting without saving.\n");
                break;
            default:
                printf("Unknown command. Press 'h' for help.\n");
                break;
        }
    }
    
    //Cleaning up so when we restart no residual hangovers.
    cleanup_system();
    printf("System terminated.\n");
    
    return 0;
}