#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/syscall.h>
#include <time.h>

int **elements1; // Store first file numbers
int **elements2; // Store second file numbers
long long **answer; // Store the answer of matrix multiplication
int col1 = 0; // Store first file column
int row1 = 0; // Store first file row
int col2 = 0; // Store second file column
int row2 = 0; // Store second file row
int number = 0; // Store the number of threads

pthread_mutex_t mutex; // Initial the lock of mutex

void read_file(char *path, int who);
void* matrix_multiplication(void *nb);
long long compute(int col, int row);


int main(int argc, char *argv[]) {
    pthread_t t;
    
    char *path1 = (char*)malloc(sizeof(char) * 1024);
    strcpy(path1, argv[2]);
    char *path2 = (char*)malloc(sizeof(char) * 1024);
    strcpy(path2, argv[3]);
    read_file(path1, 1);
    read_file(path2, 2);
    
    
    int i;
    number = atoi(argv[1]);

    answer = (long long**)malloc(sizeof(long long*) * col1);
    int j = 0;
    for(i = 0; i < col1; i++){
        answer[i] = (long long*)malloc(sizeof(long long) * row2);
        for(j = 0; j < row2; j++){
            answer[i][j] = 0;
        }
    }
    clock_t start, end;
    time(&start);
    for(i = 0; i < number; i++){
        int *arg = (int*)malloc(sizeof(int*));
        *arg = i;
        pthread_create(&t, NULL, matrix_multiplication, *arg);
    }
    
    for(i = 0; i < number; i++){
        pthread_join(t, NULL);
    }
    time(&end);
    double total = (double)(end - start);
    printf("Totaltime: %lf\n", total);
    FILE *result = fopen("result.txt", "w");
    fprintf(result, "%d %d\n", col1, row2);
    for(i = 0; i < col1; i++){
        for(j = 0; j < row2; j++){
            fprintf(result, "%lld ", answer[i][j]);
        }
        fprintf(result, "\n");
    }
    
    printf("PID:%d\n", getpid());

    FILE *file = fopen("/proc/thread_info", "r");
    char read[150];
    size_t size;
    
    while(fgets(read, 65, file) != NULL){
        printf("%s", read);
    }
    return 0;
}



void read_file(char *path, int who){
    FILE *file = fopen(path, "r");

    char *matrix_size = (char*)malloc(sizeof(char) * 1024);
    size_t size;
    getline(&matrix_size, &size, file);
    
    char *delim = " \n";
    char *col_str = strtok(matrix_size, delim);
    char *row_str = strtok(NULL, delim);
    int col = atoi(col_str), row = atoi(row_str);
    
    char *content = (char*)malloc(sizeof(char) * INT_MAX);

    if(who == 1){
        col1 = col;
        row1 = row;
        elements1 = (int**)malloc(sizeof(int*) * col);
        for(int j = 0; j < col; j++){
            elements1[j] = (int*)malloc(sizeof(int) * row);
            getline(&content, &size, file);
            char *token = strtok(content, delim);
            for(int k = 0; k < row; k++){
                elements1[j][k] = atoi(token);
                token = strtok(NULL, delim);
            }
        }
    } else if(who == 2){
        col2 = col;
        row2 = row;
        elements2 = (int**)malloc(sizeof(int*) * col);
        for(int j = 0; j < col; j++){
            elements2[j] = (int*)malloc(sizeof(int) * row);
            getline(&content, &size, file);
            char *token = strtok(content, delim);
            for(int k = 0; k < row; k++){
                elements2[j][k] = atoi(token);
                token = strtok(NULL, delim);
            }
        }
    }
    
    // test_print(col, row, who);
}

void *matrix_multiplication(void *nb){
    int index = (int*)nb;
    
    for(int i = 0; i < col1; i++){
        for(int j = 0 ; j < row2; j++){
            if((i * row2 + j) %  number == index){
                answer[i][j] = compute(i, j);
            }
        }
    }   

    pthread_mutex_lock(&mutex);
    FILE *file = fopen("//proc//thread_info", "w");
    pid_t tid = syscall(SYS_gettid);
    printf("%d\n", tid);
    fprintf(file,"%d", tid);
    fclose(file);
    pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
}

long long compute(int col, int row){
    long long value = 0;
    for(int i = 0, j = 0; i < row1; i++, j++){
        value += elements1[col][i] * elements2[j][row];
    }
    return value;
}