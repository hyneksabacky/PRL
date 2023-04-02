#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>

int parse_file(int *numbers_len, int *numbers, int size) {
    FILE *fp;
    char buff[255];
    int fsize = 0;
    int i = 0;

    fp = fopen("numbers", "r");

    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    while (!feof(fp)) {
        buff[i] = fgetc(fp);
        i++;
    }

    fclose(fp);

    if (fsize % size != 0) return 1;
    
    while (buff[*numbers_len] != '\0') {
        numbers[*numbers_len] = buff[*numbers_len];
        (*numbers_len)++;
        if (*numbers_len>65) return 1;
    }

    if (*numbers_len < 8)
        return 1;
    
    return 0;
}

void print_list(const char name, int *list, int len) {
    printf("%c:\n[", name);
    if (len > 0) {
        printf("%d", list[0]);
        if (len > 1) {
            for (int i = 0; i < len; i++) {
                printf(", %d", list[i]);
            }
        }
    }
    printf("]\n\n");
}

int main (int argc, char *argv[]) {
    int rank, size;

    int recvbuf[255];

    int numbers[255];

    int L[255];
    int L_len = 0;
    int L_res[255];

    int E[255];
    int E_len = 0;
    int E_res[255];

    int G[255];
    int G_len = 0;
    int G_res[255];

    int numbers_len = 0;
    int median = 0;

    // Initialize the MPI environment
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        if (parse_file(&numbers_len, numbers, size)) {
            MPI_Abort(MPI_COMM_WORLD, -1);
            return -1;
        }
        median = numbers[numbers_len/2 + numbers_len%2];
        numbers_len = (numbers_len)/size;
    }

    MPI_Bcast(&median,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Bcast(&numbers_len,1,MPI_INT,0,MPI_COMM_WORLD);

    MPI_Scatter(numbers, numbers_len, MPI_INT, recvbuf, numbers_len, MPI_INT, 0, MPI_COMM_WORLD);

    for (int i = 0; i < numbers_len; i++) {
        if (recvbuf[i] < median) {
            L[L_len] = recvbuf[i];
            L_len++;
        } else if (recvbuf[i] == median) {
            E[E_len] = recvbuf[i];
            E_len++;
        } else {
            G[G_len] = recvbuf[i];
            G_len++;
        }   
    }

    int *L_recvcounts = NULL;
    int *E_recvcounts = NULL;
    int *G_recvcounts = NULL;

    if (rank == 0) {
        L_recvcounts = (int*)malloc(size * sizeof(int));
        E_recvcounts = (int*)malloc(size * sizeof(int));
        G_recvcounts = (int*)malloc(size * sizeof(int));
    }

    MPI_Gather(&L_len, 1, MPI_INT,
               L_recvcounts, 1, MPI_INT,
               0, MPI_COMM_WORLD);
               
    MPI_Gather(&E_len, 1, MPI_INT,
               E_recvcounts, 1, MPI_INT,
               0, MPI_COMM_WORLD);
    
    MPI_Gather(&G_len, 1, MPI_INT,
               G_recvcounts, 1, MPI_INT,
               0, MPI_COMM_WORLD);

    int L_totlen = 0;
    int E_totlen = 0;
    int G_totlen = 0;

    int *L_displs = NULL;
    int *E_displs = NULL;
    int *G_displs = NULL;

    if (rank == 0) {
        L_displs = (int*)malloc(size * sizeof(int));
        E_displs = (int*)malloc(size * sizeof(int));
        G_displs = (int*)malloc(size * sizeof(int));

        L_displs[0] = 0;
        E_displs[0] = 0;
        G_displs[0] = 0;

        L_totlen += L_recvcounts[0];
        E_totlen += E_recvcounts[0];
        G_totlen += G_recvcounts[0];

        for (int i=1; i<size; i++) {
           L_totlen += L_recvcounts[i];
           L_displs[i] = L_displs[i-1] + L_recvcounts[i-1];

           E_totlen += E_recvcounts[i];
           E_displs[i] = E_displs[i-1] + E_recvcounts[i-1];

           G_totlen += G_recvcounts[i];
           G_displs[i] = G_displs[i-1] + G_recvcounts[i-1];
        }
    }
    
    MPI_Gatherv(L, L_len, MPI_INT,
                L_res, L_recvcounts, L_displs, MPI_INT,
                0, MPI_COMM_WORLD);

    MPI_Gatherv(E, E_len, MPI_INT,
                E_res, E_recvcounts, E_displs, MPI_INT,
                0, MPI_COMM_WORLD);

    MPI_Gatherv(G, G_len, MPI_INT,
                G_res, G_recvcounts, G_displs, MPI_INT,
                0, MPI_COMM_WORLD);

    if (rank == 0) {

        print_list('L', L_res, L_totlen);
        print_list('E', E_res, E_totlen);
        print_list('G', G_res, G_totlen);

        free(L_recvcounts);
        free(L_displs);

        free(E_recvcounts);
        free(E_displs);

        free(G_recvcounts);
        free(G_displs);
    }

	// Finalize the MPI environment
	MPI_Finalize();

	return 0;
}