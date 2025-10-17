#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#define MAX_NODES 10
#define INF INT_MAX

//normal strct as usual. For user
typedef struct {
    char name[20];
    int id;
} Location;

//Struct for locatio and (road)travel network
typedef struct {
    int locationCount;
    Location locations[MAX_NODES];
    int travelTime[MAX_NODES][MAX_NODES];
} RoadNetwork;

//make the road setup at startup
void setupRoadNetwork(RoadNetwork* network) {
    network->locationCount = 0;
    for (int i = 0; i < MAX_NODES; i++) {
        for (int j = 0; j < MAX_NODES; j++) {
            network->travelTime[i][j] = (i == j) ? 0 : INF;
        }
    }
}

int addLocation(RoadNetwork* network, const char* locationName) {
    if (network->locationCount >= MAX_NODES) {
        return -1;
    }
    
    for (int i = 0; i < network->locationCount; i++) {
        if (strcmp(network->locations[i].name, locationName) == 0) {
            return i;
        }
    }
    
    strcpy(network->locations[network->locationCount].name, locationName);
    network->locations[network->locationCount].id = network->locationCount;
    return network->locationCount++;
}

//normal connections
void connectLocations(RoadNetwork* network, const char* from, const char* to, int time) {
    int fromId = addLocation(network, from);
    int toId = addLocation(network, to);
    
    if (fromId != -1 && toId != -1) {
        network->travelTime[fromId][toId] = time;
        network->travelTime[toId][fromId] = time;
    }
}

//Finding defined ID (endpoint)
int findLocationId(RoadNetwork* network, const char* locationName) {
    for (int i = 0; i < network->locationCount; i++) {
        if (strcmp(network->locations[i].name, locationName) == 0) {
            return i;
        }
    }
    return -1;
}

//The fastest route from given destinations
void calculateFastestRoute(RoadNetwork* network, int startId, int* shortestTime, int* previousLocation) {
    int visited[MAX_NODES] = {0};
    
    for (int i = 0; i < network->locationCount; i++) {
        shortestTime[i] = INF;
        previousLocation[i] = -1;
    }
    shortestTime[startId] = 0;
    
    for (int count = 0; count < network->locationCount - 1; count++) {
        int minTime = INF;
        int currentId = -1;
        
        for (int i = 0; i < network->locationCount; i++) {
            if (!visited[i] && shortestTime[i] <= minTime) {
                minTime = shortestTime[i];
                currentId = i;
            }
        }
        
        if (currentId == -1) break;
        visited[currentId] = 1;
        
        for (int i = 0; i < network->locationCount; i++) {
            if (!visited[i] && network->travelTime[currentId][i] != INF && 
                shortestTime[currentId] != INF && 
                shortestTime[currentId] + network->travelTime[currentId][i] < shortestTime[i]) {
                shortestTime[i] = shortestTime[currentId] + network->travelTime[currentId][i];
                previousLocation[i] = currentId;
            }
        }
    }
}

void showRoute(RoadNetwork* network, int* previousLocation, int currentId) {
    if (previousLocation[currentId] == -1) {
        printf("%s", network->locations[currentId].name);
        return;
    }
    showRoute(network, previousLocation, previousLocation[currentId]);
    printf(" -> %s", network->locations[currentId].name);
}

//A step above, displaying emergency route
void displayEmergencyRoute(RoadNetwork* network, const char* startPoint) {
    int startId = findLocationId(network, startPoint);
    int emergencyId = findLocationId(network, "Emergency Site");
    
    if (startId == -1) {
        printf("Error: point of '%s' not found!\n", startPoint);
        return;
    }
    
    if (emergencyId == -1) {
        printf("Error, Site not found!\n");
        return;
    }
    
    int shortestTime[MAX_NODES];
    int previousLocation[MAX_NODES];
    
    calculateFastestRoute(network, startId, shortestTime, previousLocation);
    
    if (shortestTime[emergencyId] == INF) {
        printf("No route available from %s!\n", startPoint);
        return;
    }
    
    printf("\n!!!!! FASTEST EMERGENCY ROUTE !!!!!\n");
    printf("From: %s\n", startPoint);
    printf("To: Emergency Site\n");
    printf("Route: ");
    showRoute(network, previousLocation, emergencyId);
    printf("\nTotal Time: %d minutes\n", shortestTime[emergencyId]);
}

void createCityMap(RoadNetwork* network) {
    connectLocations(network, "Dispatch Center", "Sector A", 10);
    connectLocations(network, "Dispatch Center", "Sector D", 30);
    connectLocations(network, "Sector A", "Sector B", 10);
    connectLocations(network, "Sector B", "Emergency Site", 15);
    connectLocations(network, "Sector D", "Emergency Site", 5);
    connectLocations(network, "Sector B", "Junction C", 3);
    connectLocations(network, "Junction C", "Sector E", 6);
    connectLocations(network, "Sector E", "Emergency Site", 4);
}

void showAvailableStarts(RoadNetwork* network) {
    printf("\nAvailable starting points:\n");
    for (int i = 0; i < network->locationCount; i++) {
        if (strcmp(network->locations[i].name, "Emergency Site") != 0) {
            printf("- %s\n", network->locations[i].name);
        }
    }
}

int main() {
    RoadNetwork city;
    setupRoadNetwork(&city);
    createCityMap(&city);
    printf("Emergency Route Finder\n");
    
    char startPoint[50];
    int userChoice;
    
    do {
        printf("\nOptions:\n");
        printf("1. Finding fastest route to Emergency Site\n");
        printf("2. Available starting points\n");
        printf("3. Quit\n");
        printf("Choose option: ");
        
        if (scanf("%d", &userChoice) != 1) {
            printf("Invalid input! Enter a number!\n");
            while (getchar() != '\n');
            continue;
        }
        
        switch (userChoice) {
            case 1:
                showAvailableStarts(&city);
                printf("\nEnter Start Point: ");
                getchar();
                fgets(startPoint, sizeof(startPoint), stdin);
                startPoint[strcspn(startPoint, "\n")] = 0;
                
                displayEmergencyRoute(&city, startPoint);
                break;
                
            case 2:
                showAvailableStarts(&city);
                break;
                
            case 3:
                printf("Exiting! Safe!!\n");
                break;
                
            default:
                printf("Invalid choice! Enter 1, 2, or 3.\n");
        }
    } while (userChoice != 3);
    
    return 0;
}