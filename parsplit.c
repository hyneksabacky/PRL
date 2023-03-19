#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>

void parse_file(int *numbers_len, int *numbers, int size) {
    FILE *fp;
    char buff[255];

    fp = fopen("numbers", "r");
    fscanf(fp, "%s", buff);

    while (buff[*numbers_len] != '\0') {
        numbers[*numbers_len] = buff[*numbers_len];
        printf("%d, ", numbers[*numbers_len]);
        (*numbers_len)++;
        if (*numbers_len>10000) break;
    }

    fclose(fp);
}

int main (int argc, char *argv[]) {
    int rank, size;

    int recvbuf[255];

    int numbers[255];
    int numbers_len = 0;
    int median = 0;

    // Initialize the MPI environment
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        parse_file(&numbers_len, numbers, size);
        median = numbers[numbers_len/2 + numbers_len%2];
        numbers_len = (numbers_len)/size;
        printf(" -> median = %d\n", median);
        printf("numbers_len/size = %d\n", numbers_len);
    }

    MPI_Bcast(&median,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Bcast(&numbers_len,1,MPI_INT,0,MPI_COMM_WORLD);

    //printf("%d. muj median = %d\n", rank, median);

    MPI_Scatter(numbers, numbers_len, MPI_INT, recvbuf, numbers_len, MPI_INT, 0, MPI_COMM_WORLD);

    printf("Rank %d received array: ", rank);
    for (int i = 0; i < numbers_len; i++) {
        printf("%d ", recvbuf[i]);
    }
    printf("\n");

	// Finalize the MPI environment
	MPI_Finalize();

	return 0;
}