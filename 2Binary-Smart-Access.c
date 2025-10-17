#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define MAX_NAMES 40
#define MAX_NAME_LENGTH 50
#define UNKNOWN_NAMES_FILE "unknown_names.log"

//BST node struc
typedef struct TreeNode {
    char name[MAX_NAME_LENGTH];
    struct TreeNode* left;
    struct TreeNode* right;
} TreeNode;

//Function declarations ofc
TreeNode* createNode(const char* name);
TreeNode* insertBST(TreeNode* root, const char* name);
TreeNode* searchBST(TreeNode* root, const char* name);
void freeBST(TreeNode* root);
int levenshteinDistance(const char* s1, const char* s2);
void findClosestMatch(TreeNode* root, const char* input, char* bestMatch, int* bestDistance);
void logUnknownName(const char* name);
TreeNode* loadNamesFromFile(const char* filename);
void displayMenu();
void processAccessRequest(TreeNode* root);
void printTree(TreeNode* root, int space);



TreeNode* createNode(const char* name) {
    TreeNode* newNode = (TreeNode*)malloc(sizeof(TreeNode));
    if (newNode != NULL) {
        strncpy(newNode->name, name, MAX_NAME_LENGTH - 1);
        newNode->name[MAX_NAME_LENGTH - 1] = '\0';
        newNode->left = newNode->right = NULL;
    }
    return newNode;
}

//Inserting name into BST (sensitiveee)
TreeNode* insertBST(TreeNode* root, const char* name) {
    if (root == NULL) {
        return createNode(name);
    }
    
    int cmp = strcmp(name, root->name);
    if (cmp < 0) {
        root->left = insertBST(root->left, name);
    } else if (cmp > 0) {
        root->right = insertBST(root->right, name);
    }
    
    return root;
}


TreeNode* searchBST(TreeNode* root, const char* name) {
    if (root == NULL) return NULL;
    
    int cmp = strcmp(name, root->name);
    if (cmp == 0) return root;
    
    if (cmp < 0) return searchBST(root->left, name);
    else return searchBST(root->right, name);
}

//Freeing memory once the BST operations are done
void freeBST(TreeNode* root) {
    if (root != NULL) {
        freeBST(root->left);
        freeBST(root->right);
        free(root);
    }
}

//Complicated  Levenshtein distance calculation
int levenshteinDistance(const char* s1, const char* s2) {
    int len1 = strlen(s1);
    int len2 = strlen(s2);

    int** dp = (int**)malloc((len1 + 1) * sizeof(int*));
    for (int i = 0; i <= len1; i++) {
        dp[i] = (int*)malloc((len2 + 1) * sizeof(int));
    }
    
    for (int i = 0; i <= len1; i++) {
        dp[i][0] = i;
    }
    for (int j = 0; j <= len2; j++) {
        dp[0][j] = j;
    }
    
    //Filling prerequisite DP table
    for (int i = 1; i <= len1; i++) {
        for (int j = 1; j <= len2; j++) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            dp[i][j] = INT_MAX;
            dp[i][j] = (dp[i-1][j] + 1 < dp[i][j]) ? dp[i-1][j] + 1 : dp[i][j];
            dp[i][j] = (dp[i][j-1] + 1 < dp[i][j]) ? dp[i][j-1] + 1 : dp[i][j];
            dp[i][j] = (dp[i-1][j-1] + cost < dp[i][j]) ? dp[i-1][j-1] + cost : dp[i][j];
        }
    }
    
    int result = dp[len1][len2];
    
    //Freeing memory once functioanlity is done
    for (int i = 0; i <= len1; i++) {
        free(dp[i]);
    }
    free(dp);
    
    return result;
}

//Closest match in BST
void findClosestMatch(TreeNode* root, const char* input, char* bestMatch, int* bestDistance) {
    if (root == NULL) return;
    
    int distance = levenshteinDistance(input, root->name);
    if (distance < *bestDistance) {
        *bestDistance = distance;
        strncpy(bestMatch, root->name, MAX_NAME_LENGTH - 1);
        bestMatch[MAX_NAME_LENGTH - 1] = '\0';
    }
    
    findClosestMatch(root->left, input, bestMatch, bestDistance);
    findClosestMatch(root->right, input, bestMatch, bestDistance);
}

//Logging unknown name to file
void logUnknownName(const char* name) {
    FILE* file = fopen(UNKNOWN_NAMES_FILE, "a");
    if (file != NULL) {
        fprintf(file, "%s\n", name);
        fclose(file);
        printf("Unknown name '%s' has been logged for reviewing.\n", name);
    } else {
        printf("Error couldn't log name\n");
    }
}

//Loading names from file
TreeNode* loadNamesFromFile(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error: Couldn't not open file '%s'. Make sure it really doest exist\n", filename);
        return NULL;
    }
    
    TreeNode* root = NULL;
    char name[MAX_NAME_LENGTH];
    int count = 0;
    
    printf("Loading names from '%s'...\n", filename);
    
    while (fgets(name, sizeof(name), file) != NULL && count < MAX_NAMES) {
        //Remove newline character. because YAH we got to.
        name[strcspn(name, "\n")] = '\0';
        
        if (strlen(name) > 0) {
            root = insertBST(root, name);
            count++;
            printf("Loaded: %s\n", name);
        }
    }
    
    fclose(file);
    printf("Successfully loaded %d names.\n", count);
    return root;
}

// Display menu options
void displayMenu() {
    printf("\n!!!!! Binary Access Control System !!!!!\n");
    printf("1. Request Access\n");
    printf("2. Display Authorized Names (BST)\n");
    printf("3. Exit\n");
    printf("Choose an option (1-3): ");
}

//Access request from user
void processAccessRequest(TreeNode* root) {
    char input[MAX_NAME_LENGTH];
    
    printf("\nEnter your name: ");
    if (fgets(input, sizeof(input), stdin) == NULL) {
        printf("Error reading\n");
        return;
    }
    
    input[strcspn(input, "\n")] = '\0';
    
    if (strlen(input) == 0) {
        printf("Error: No name entered.\n");
        return;
    }
    
    printf("Processing: '%s'\n", input);
    
    TreeNode* result = searchBST(root, input);
    if (result != NULL) {
        printf("ACCESS GRANTED, %s!\n", result->name);
        return;
    }
    
    char bestMatch[MAX_NAME_LENGTH] = "";
    int bestDistance = INT_MAX;
    
    findClosestMatch(root, input, bestMatch, &bestDistance);
    
    if (bestDistance <= 3 && bestDistance > 0) { 
        printf("ACCESS DENIED!!!\n");
        printf("💡 Did you mean: %s? (Levenshtein distance: %d)\n", bestMatch, bestDistance);
    } else {
        printf("✗ ACCESS DENIED: Name not recognized.\n");
        printf("Could not be found in dat\n");
    }
    
    //Logging unknown name
    logUnknownName(input);
}

//Print BST like an actual tree
void printTree(TreeNode* root, int space) {
    if (root == NULL) return;
    
    //Increasing distance between levels
    space += 5;
    
    printTree(root->right, space);
    
    printf("\n");
    for (int i = 5; i < space; i++) {
        printf(" ");
    }
    printf("%s\n", root->name);
    
    //Process the left child LAST
    printTree(root->left, space);
}

int main() {
    TreeNode* root = NULL;
    char filename[100];
    int choice;
    
    printf("SYSTEM IS STARTING\n");
    printf("Enter the filename containing authorized names: ");
    
    if (fgets(filename, sizeof(filename), stdin) == NULL) {
        printf("Error reading filenames\n");
        return 1;
    }
    
    filename[strcspn(filename, "\n")] = '\0';
    
    //Loading names from file
    root = loadNamesFromFile(filename);
    if (root == NULL) {
        printf("Failing to load names...oh well exiting.\n");
        return 1;
    }
    
    //Loop through main program to do once again when done.
    while (1) {
        displayMenu();
        
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input\n");
            while (getchar() != '\n'); 
            continue;
        }
        //this actually clears input buffer because new line
        while (getchar() != '\n');
        
        switch (choice) {
            case 1:
                processAccessRequest(root);
                break;
            case 2:
                printf("\nNames, BST Structure:\n");
                printTree(root, 0);
                break;
            case 3:
                printf("Goodbye!\n");
                freeBST(root);
                return 0;
            default:
                printf("Invalid choice. Please select 1, 2, or 3.\n");
        }
    }
    //Mandatory freeing again
    freeBST(root);
    return 0;
}