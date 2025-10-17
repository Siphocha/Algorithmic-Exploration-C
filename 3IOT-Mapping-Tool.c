#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_DEVICES 100
#define DEVICE_ID_LENGTH 5

//Business as usual, Strucs are delusional.
typedef struct {
    char id[DEVICE_ID_LENGTH];
    int index;
} Device;

//Struc for preliminary devices.
typedef struct {
    Device devices[MAX_DEVICES];
    int adjacency_matrix[MAX_DEVICES][MAX_DEVICES];
    int bidirectional[MAX_DEVICES][MAX_DEVICES];
    int device_count;
} Graph;

//Function Prototypes
void initialize_graph(Graph *g);
int find_device_index(Graph *g, const char *device_id);
int add_device(Graph *g, const char *device_id);
void add_connection(Graph *g, const char *from, const char *to, int is_bidirectional);
void display_adjacency_matrix(Graph *g);
void query_device_connections(Graph *g, const char *device_id);
void display_all_connections(Graph *g, const char *device_id);
void remove_connection(Graph *g, const char *from, const char *to);
void remove_device(Graph *g, const char *device_id);
void print_menu();

void initialize_graph(Graph *g) {
    g->device_count = 0;
    for (int i = 0; i < MAX_DEVICES; i++) {
        for (int j = 0; j < MAX_DEVICES; j++) {
            g->adjacency_matrix[i][j] = 0;
            g->bidirectional[i][j] = 0;
        }
    }
}


//Device ordering and numeric attachement
int find_device_index(Graph *g, const char *device_id) {
    for (int i = 0; i < g->device_count; i++) {
        if (strcmp(g->devices[i].id, device_id) == 0) {
            return i;
        }
    }
    return -1;
}

//Once device index known. add it.
int add_device(Graph *g, const char *device_id) {
    if (g->device_count >= MAX_DEVICES) {
        printf("Error: Maximum device limit reached!\n");
        return -1;
    }
    
    if (find_device_index(g, device_id) != -1) {
        return find_device_index(g, device_id);
    }
    
    strcpy(g->devices[g->device_count].id, device_id);
    g->devices[g->device_count].index = g->device_count;
    g->device_count++;
    return g->device_count - 1;
}

void add_connection(Graph *g, const char *from, const char *to, int is_bidirectional) {
    int from_index = add_device(g, from);
    int to_index = add_device(g, to);
    
    if (from_index == -1 || to_index == -1) {
        printf("Error: Cannot add connection - device limit reached.\n");
        return;
    }
    
    g->adjacency_matrix[from_index][to_index] = 1;
    
    if (is_bidirectional) {
        g->adjacency_matrix[to_index][from_index] = 1;
        g->bidirectional[from_index][to_index] = 1;
        g->bidirectional[to_index][from_index] = 1;
    }
}

void display_adjacency_matrix(Graph *g) {
    if (g->device_count == 0) {
        printf("No devices in the graph.\n");
        return;
    }
    
    printf("\n!!!!! ADJACENCY MATRIX !!!!!\n");
    
    // Print header row
    printf("     ");
    for (int i = 0; i < g->device_count; i++) {
        printf("%-5s", g->devices[i].id);
    }
    printf("\n");
    
    // Print matrix rows
    for (int i = 0; i < g->device_count; i++) {
        printf("%-5s", g->devices[i].id);
        for (int j = 0; j < g->device_count; j++) {
            if (g->bidirectional[i][j]) {
                printf("B    ");
            } else {
                printf("%d    ", g->adjacency_matrix[i][j]);
            }
        }
        printf("\n");
    }
    printf("(B = Bidirectional connection)\n");
}

void query_device_connections(Graph *g, const char *device_id) {
    int device_index = find_device_index(g, device_id);
    
    if (device_index == -1) {
        printf("Error, Device '%s' not found on the network\n", device_id);
        return;
    }
    
    printf("\n!!!!! CONNECTIONS FOR %s !!!!!\n", device_id);
    
    //Finding device this sends to.
    printf("Outgoing connections (sends to): ");
    int outgoing_count = 0;
    for (int i = 0; i < g->device_count; i++) {
        if (g->adjacency_matrix[device_index][i] && !g->bidirectional[device_index][i]) {
            printf("%s ", g->devices[i].id);
            outgoing_count++;
        }
    }
    if (outgoing_count == 0) printf("None.");
    printf("\n");
    
    //Find incoming connections (recieving end)
    printf("Incoming connections (receives from): ");
    int incoming_count = 0;
    for (int i = 0; i < g->device_count; i++) {
        if (g->adjacency_matrix[i][device_index] && !g->bidirectional[i][device_index]) {
            printf("%s ", g->devices[i].id);
            incoming_count++;
        }
    }
    if (incoming_count == 0) printf("None");
    printf("\n");
    
    printf("Bidirectional connections: ");
    int bidirectional_count = 0;
    for (int i = 0; i < g->device_count; i++) {
        if (g->bidirectional[device_index][i]) {
            printf("%s ", g->devices[i].id);
            bidirectional_count++;
        }
    }
    if (bidirectional_count == 0) printf("None");
    printf("\n");
}

void display_all_connections(Graph *g, const char *device_id) {
    int device_index = find_device_index(g, device_id);
    
    if (device_index == -1) {
        printf("Error: Device '%s' not found in the network.\n", device_id);
        return;
    }
    
    printf("\n!!!!! ALL DIRECT CONNECTIONS FOR %s !!!!!\n", device_id);
    
    int connection_count = 0;
    for (int i = 0; i < g->device_count; i++) {
        if (g->adjacency_matrix[device_index][i] || g->adjacency_matrix[i][device_index]) {
            const char* direction = "";
            if (g->bidirectional[device_index][i]) {
                direction = " <-> (bidirectional)";
            } else if (g->adjacency_matrix[device_index][i]) {
                direction = " -> (sends to)";
            } else {
                direction = " <- (receives from)";
            }
            printf("- %s%s\n", g->devices[i].id, direction);
            connection_count++;
        }
    }
    
    if (connection_count == 0) {
        printf("No connections found\n");
    }
}

void remove_connection(Graph *g, const char *from, const char *to) {
    int from_index = find_device_index(g, from);
    int to_index = find_device_index(g, to);
    
    if (from_index == -1 || to_index == -1) {
        printf("Error none of the devices have been found\n");
        return;
    }
    
    if (g->bidirectional[from_index][to_index]) {
        g->adjacency_matrix[from_index][to_index] = 0;
        g->adjacency_matrix[to_index][from_index] = 0;
        g->bidirectional[from_index][to_index] = 0;
        g->bidirectional[to_index][from_index] = 0;
        printf("Bidirectional connection between %s and %s stopped.\n", from, to);
    } else if (g->adjacency_matrix[from_index][to_index]) {
        g->adjacency_matrix[from_index][to_index] = 0;
        printf("Connection from %s to %s removed.\n", from, to);
    } else {
        printf("Error: No connection found from %s to %s.\n", from, to);
    }
}

void remove_device(Graph *g, const char *device_id) {
    int device_index = find_device_index(g, device_id);
    
    if (device_index == -1) {
        printf("Error: Device '%s' not found.\n", device_id);
        return;
    }
    
    //Removed all connections involving this device
    for (int i = 0; i < g->device_count; i++) {
        g->adjacency_matrix[device_index][i] = 0;
        g->adjacency_matrix[i][device_index] = 0;
        g->bidirectional[device_index][i] = 0;
        g->bidirectional[i][device_index] = 0;
    }
    
    //Shifting device array to remove and make space for other.
    for (int i = device_index; i < g->device_count - 1; i++) {
        g->devices[i] = g->devices[i + 1];
        g->devices[i].index = i;
    }
    
    //Update adjacency matrix ( a bit verbose at this point but we emulating IOT)
    for (int i = 0; i < g->device_count - 1; i++) {
        for (int j = device_index; j < g->device_count - 1; j++) {
            g->adjacency_matrix[i][j] = g->adjacency_matrix[i][j + 1];
            g->bidirectional[i][j] = g->bidirectional[i][j + 1];
        }
    }
    for (int i = device_index; i < g->device_count - 1; i++) {
        for (int j = 0; j < g->device_count - 1; j++) {
            g->adjacency_matrix[i][j] = g->adjacency_matrix[i + 1][j];
            g->bidirectional[i][j] = g->bidirectional[i + 1][j];
        }
    }
    
    g->device_count--;
    printf("Device '%s' removed successfully.\n", device_id);
}

void print_menu() {
    printf("\n!!!!! IoT DEVICE COMMUNICATION MAPPING TOOL !!!!!\n");
    printf("1. Adjacency Matrix View\n");
    printf("2. Query Device Connections\n");
    printf("3. Show All Direct Connections for a Device\n");
    printf("4. Adding New Connection\n");
    printf("5. Adding Bidirectional Connection\n");
    printf("6. Removing Connection\n");
    printf("7. Removing Device\n");
    printf("8. Display All Devices\n");
    printf("9. Exit\n");
    printf("Enter your choice (1-9): ");
}

void initialize_default_connections(Graph *g) {
    //These are the default connections from problem statement.
    add_connection(g, "D001", "D002", 0);
    add_connection(g, "D001", "D003", 0);
    add_connection(g, "D002", "D004", 0);
    add_connection(g, "D003", "D005", 0);
    add_connection(g, "D004", "D005", 0);
    add_connection(g, "D004", "D006", 0);
    add_connection(g, "D005", "D007", 0);
    add_connection(g, "D006", "D008", 0);
    
    printf("Default IoT device connections initialized:\n");
    printf("D001 -> D002, D001 -> D003, D002 -> D004, D003 -> D005\n");
    printf("D004 -> D005, D004 -> D006, D005 -> D007, D006 -> D008\n");
}

int main() {
    Graph device_graph;
    initialize_graph(&device_graph);
    
    //Initializing with default connections
    initialize_default_connections(&device_graph);
    
    int choice;
    char device_id[DEVICE_ID_LENGTH];
    char from_device[DEVICE_ID_LENGTH];
    char to_device[DEVICE_ID_LENGTH];
    
    do {
        print_menu();
        scanf("%d", &choice);
        getchar(); 
        
        switch (choice) {
            case 1:
                display_adjacency_matrix(&device_graph);
                break;
                
            case 2:
                printf("Device ID to query (e.g., D004): ");
                scanf("%4s", device_id);
                query_device_connections(&device_graph, device_id);
                break;
                
            case 3:
                printf("Device ID to see all connections: ");
                scanf("%4s", device_id);
                display_all_connections(&device_graph, device_id);
                break;
                
            case 4:
                printf("Enter source device ID: ");
                scanf("%4s", from_device);
                printf("Enter destination device ID: ");
                scanf("%4s", to_device);
                add_connection(&device_graph, from_device, to_device, 0);
                printf("Unidirectional connection from %s to %s added.\n", from_device, to_device);
                break;
                
            case 5:
                printf("Enter first device ID: ");
                scanf("%4s", from_device);
                printf("Enter second device ID: ");
                scanf("%4s", to_device);
                add_connection(&device_graph, from_device, to_device, 1);
                printf("Bidirectional connection between %s and %s added.\n", from_device, to_device);
                break;
                
            case 6:
                printf("Enter source device ID: ");
                scanf("%4s", from_device);
                printf("Enter destination device ID: ");
                scanf("%4s", to_device);
                remove_connection(&device_graph, from_device, to_device);
                break;
                
            case 7:
                printf("Enter device ID to remove: ");
                scanf("%4s", device_id);
                remove_device(&device_graph, device_id);
                break;
                
            case 8:
                printf("\n!!!!! ALL DEVICES IN NETWORK !!!!!\n");
                if (device_graph.device_count == 0) {
                    printf("No devices in the network, on the network currently\n");
                } else {
                    for (int i = 0; i < device_graph.device_count; i++) {
                        printf("%s\n", device_graph.devices[i].id);
                    }
                    printf("Total devices: %d\n", device_graph.device_count);
                }
                break;
                
            case 9:
                printf("Goodbye!\n");
                break;
                
            default:
                printf("Enter a number between 1-9.\n");
                break;
        }
        
    } while (choice != 9);
    
    return 0;
}