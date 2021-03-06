#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "datatypes/list.h"
#include "time/hr_time.h"

#include <gmp.h>
#include <mpi.h>

#define MASTER( _RANK_ ) ((_RANK_)==0)

hr_timer_t hr_tm;
list_t *factors;

mpz_t n;  //number
char *nstr; //string of the number
int nlen;  //length of the string nstr


MPI_Status status;
MPI_Request req;
int tag;

void random_number(mpz_t n, int numlen);
void calculate_interval(int rank, mpz_t n_interval, mpz_t n_start, mpz_t n_end );
void print_list(list_t *list);

int has_process_up(char *process, int nprocs);

void read_parameters(int argc, char** argv);

void master_begin();
void master_end();

void search_factors(int rank, int nprocs);

void stop_node_search(char *process, int nprocs);

int main(int argc, char** argv)
{
   int rank, nprocs;

   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   tag = 1234;

   mpz_init(n);
   nstr = 0;

   if(MASTER(rank)){
      read_parameters(argc, argv);
      master_begin();
   }
   
   search_factors(rank, nprocs);

   if(MASTER(rank)) master_end();

   MPI_Finalize();

   return 0;
}

void search_factors(int rank, int nprocs)
{
   int i;
   int msg_received = 0;
   int found_divisor;

   char *process;

   list_t *temp;
   node_t *node;
   
   mpz_t r; //remainder

   mpz_t d; //divisor
   char *dstr = 0;
   int dlen;

   mpz_t n_interval;
   char *nintstr = 0; //interval string
   int  nintlen;  //interval len

   mpz_t quotient, remainder;
   mpz_t n_start;
   mpz_t n_end;

   process = (char*)malloc(sizeof(char)*nprocs);
   temp = create_list();

   mpz_init(r);
   mpz_init(d);
   mpz_init(n_start);
   mpz_init(n_end);
   mpz_init(quotient);
   mpz_init(remainder);
   mpz_init(n_interval);

   while(1){
      /*
       * Pre configuracoes para iniciar a busca por um novo divisor
      */

      found_divisor = 0;
          
      MPI_Bcast(&nlen, 1, MPI_INT, 0, MPI_COMM_WORLD);

      if(nlen==-1) {
         #ifdef DEBUG
         printf("Processor %d has finished completely\n", rank);
         #endif
         break;
      }

      if( MASTER(rank) ){
         memset(process, 1, nprocs);
         process[0]=0;
      }else{
         #ifdef DEBUG
         printf("Processor %d malloc %d at line 142\n", rank, (nlen+1));
         #endif
         if(!nstr) nstr = (char*)malloc(nlen+1);
      }
       
      MPI_Bcast(nstr, (nlen+1), MPI_CHAR, 0, MPI_COMM_WORLD);
      if( !MASTER(rank) ){
         mpz_set_str(n, nstr, 10);
      }
      
      
      if( MASTER(rank) ){
         mpz_t n_sqrt;
         mpz_init(n_sqrt);
         // Define limite superior para busca => sqrt(N).
         mpz_sqrt(n_sqrt, n);
   
         // Define número de iterações que cada thread deve realizar =>
         // n_interval = sqrt(N) / numero de nodos.
         mpz_cdiv_q_ui(n_interval, n_sqrt, nprocs);
          
         nintstr = mpz_get_str(NULL, 10, n_interval);
         nintlen = strlen(nintstr);
      }
       
      MPI_Bcast(&nintlen, 1, MPI_INT, 0, MPI_COMM_WORLD);
      if( !MASTER(rank) ){
         #ifdef DEBUG
         printf("Processor %d malloc %d at line 169\n", rank, (nintlen+1));
         #endif
         nintstr = (char*)malloc(nintlen+1);
      }
       
      MPI_Bcast(nintstr, (nintlen+1), MPI_CHAR, 0, MPI_COMM_WORLD);
      if( !MASTER(rank) ){
         mpz_set_str(n_interval, nintstr, 10);
      }
       
      calculate_interval(rank, n_interval, n_start, n_end);

      //gmp_printf("Process: %d; Start: %Zd; End: %Zd; Num: %Zd\n", rank, n_start, n_end, n);
      
      // inicia a busca por algum divisor de n dentro do intervalo calculado
      for(mpz_set(quotient, n_start); mpz_cmp(n_end,quotient) >= 0; mpz_add_ui(quotient, quotient, 2)) {

         if( MASTER(rank) ){
            MPI_Iprobe(MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &msg_received, &status);
            if(msg_received){
               int proc_idx;
               MPI_Irecv(&proc_idx, 1, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &req);
               #ifdef DEBUG
               printf("Received Message %d\n", dlen);
               #endif
               if(proc_idx>0){
                  #ifdef DEBUG
                  printf("Processor %d malloc %d at line 211\n", rank, (dlen+1));
                  #endif
                  MPI_Recv(&dlen, 1, MPI_INT, proc_idx, tag, MPI_COMM_WORLD, &status);
                  process[proc_idx] = 0; // set that the node proc_idx has finished searching for a divisor
                  
                  if(dlen>0){ //divisor found with dlen digits
                     dstr = (char*)malloc(dlen+1);
                     MPI_Recv(dstr, (dlen+1), MPI_CHAR, proc_idx, tag, MPI_COMM_WORLD, &status);
                     mpz_set_str(d, dstr, 10);
                     found_divisor = 1;
   
                     ///gmp_printf("Divisor: %Zd\n", d);
                     ///gmp_printf("Divisor Length: %d\n", dlen);
                            
                     stop_node_search(process, nprocs); //finishes all nodes if a divisor was found
                     break;
                  }
               }
            }
         }else{
            MPI_Iprobe(0, tag, MPI_COMM_WORLD, &msg_received, &status);
            if(msg_received){
               MPI_Irecv(&dlen, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, &req);
               if(dlen==0){
                  #ifdef DEBUG
                  printf("Processor %d is finished\n", rank);
                  #endif
                  //printf("Finish Requested: %d\n", rank);
                  break;
               }else {
                  printf("%d: Why am I here? %d\n", rank, dlen);
                  break;
               }
            }
         }
   
         //check if the current quotient is actually a divisor
         mpz_mod(remainder, n, quotient);
         if( !mpz_sgn(remainder) ) { //if it is a divisor
            //gmp_printf("Processor %d found divisor: %Zd\n", rank, quotient);
            found_divisor = 1;
            mpz_set(d,quotient);
            if( MASTER(rank) ){
               stop_node_search(process, nprocs);
               break;
            }else {
               dstr = mpz_get_str(NULL, 10, quotient);
               dlen = strlen(dstr);
               
               // send the divisor to the master
               MPI_Isend(&rank, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, &req);
               MPI_Send(&dlen, 1, MPI_INT, 0, tag, MPI_COMM_WORLD);
               MPI_Send(dstr, (dlen+1), MPI_CHAR, 0, tag, MPI_COMM_WORLD);
               break;
            }
         }

      }//for
          
      if( MASTER(rank) ){
         while( has_process_up(process, nprocs) ){ //wait until all nodes finish their searching
            MPI_Iprobe(MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &msg_received, &status);
            if(msg_received){
               int proc_idx;
               MPI_Irecv(&proc_idx, 1, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &req);
               #ifdef DEBUG
               printf("Received Message after loop: %d\n", dlen);
               #endif
               if(proc_idx>0){
                  MPI_Recv(&dlen, 1, MPI_INT, proc_idx, tag, MPI_COMM_WORLD, &status);
                  process[proc_idx] = 0;
                  //printf("Finishing process: %d\n", proc_idx);
                  if(dlen>0){
                     dstr = (char*)malloc(dlen+1);
                     MPI_Recv(dstr, (dlen+1), MPI_CHAR, proc_idx, tag, MPI_COMM_WORLD, &status);
                     if(!found_divisor){
                        mpz_set_str(d, dstr, 10);
                        found_divisor = 1;
                            
                        ///gmp_printf("Divisor: %Zd\n", d);
                        ///gmp_printf("Divisor Length: %d\n", dlen);
                           
                        stop_node_search(process, nprocs);
                     }
                  }
               }
            }
         }

         if(found_divisor && mpz_cmp(n,d)!=0 && mpz_cmp_ui(d,1)>0){
            mpz_cdiv_q(quotient,n,d);
            mpz_set(n,quotient);
            nstr = mpz_get_str(NULL, 10, n);
            nlen = strlen(nstr);
            #ifdef DEBUG
            gmp_printf("New Number: %Zd\n", n);
            #endif
            node = alloc_node(sizeof(mpz_t));
            mpz_init(NODEGET( mpz_t, node ));
            mpz_set(NODEGET( mpz_t, node ), d);
            list_push_back(temp, node);
         }else{
            node = alloc_node(sizeof(mpz_t));
            mpz_init(NODEGET( mpz_t, node ));
            mpz_set(NODEGET( mpz_t, node ), n);
            #ifdef DEBUG
            gmp_printf("New Divisor: %Zd\n", n);
            #endif
            list_push_back(factors, node);

            node = list_pop_front(temp);
            if(node) {
               mpz_set(n, NODEGET( mpz_t, node ));
               destroy_node(&node);
               nstr = mpz_get_str(NULL, 10, n);
               nlen = strlen(nstr);
            } else {
               nlen = -1;
            }
         }
             
      }else {
         //let the master knows that the node has finished looking for a divisor
         if(!found_divisor){
            dlen = 0;
            MPI_Isend(&rank, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, &req); 
            MPI_Send(&dlen, 1, MPI_INT, 0, tag, MPI_COMM_WORLD);
         }
      }
   }//while(1)

   free(process);
   destroy_list_nodes(temp);
   destroy_list(&temp);
}

void stop_node_search(char *process, int nprocs)
{
   
   int i;
   int msg_zero = 0;
   for(i=1; i<nprocs; i++){
      if(process[i]) {
         MPI_Isend(&msg_zero, 1, MPI_INT, i, tag, MPI_COMM_WORLD, &req);   
      }
   }
}

void read_parameters(int argc, char** argv)
{
   if(argc > 2){
      if(!strcmp(argv[1], "-r")){
         random_number(n, atoi(argv[2]));
         nstr = mpz_get_str(NULL, 10, n);
         nlen = strlen(nstr);
         gmp_printf("Number: (%d) %Zd\n", nlen, n);
      }else if(!strcmp(argv[1], "-n")){
         nstr = argv[2];
         mpz_set_str(n, nstr, 10);
         nlen = strlen(nstr);
         gmp_printf("Number: (%d) %Zd\n", nlen, n);
      }else {
         printf("usage:\n mpiexec -n <number of processors> mpi_factoring -n <number>\n mpiexec -n <number of processors> mpi_factoring  -r <num_digits>\n");
      }
   }else {
      printf("usage:\n mpiexec -n <number of processors> mpi_factoring -n <number>\n mpiexec -n <number of processors> mpi_factoring  -r <num_digits>\n");
   }
}

void master_begin()
{
   mpz_t r; //remainder
   node_t *node;

   factors = create_list();

   mpz_init(r);

   hrt_start(&hr_tm); //it must be right here
   if(nlen>0){
      while(1){
         mpz_mod_ui(r, n, 2);
         if( (mpz_cmp_ui(n,2) > 0) && mpz_sgn(r)==0 ) {
            node = alloc_node(sizeof(mpz_t));
            mpz_init(NODEGET( mpz_t, node ));
            mpz_set_ui(NODEGET( mpz_t, node ), 2);
            list_push_back(factors, node);
             
            mpz_cdiv_q_ui(n,n,2);
         }else if(mpz_cmp_ui(n,2) == 0){
            node = alloc_node(sizeof(mpz_t));
            mpz_init(NODEGET( mpz_t, node ));
            mpz_set_ui(NODEGET( mpz_t, node ), 2);
            list_push_back(factors, node);
            nlen = -1;
            break;
         }else {
            nstr = mpz_get_str(NULL, 10, n);
            nlen = strlen(nstr);
            #ifdef DEBUG
            gmp_printf("New Number: %Zd\n", n);
            #endif
            break;
         }
      }
   }
}

void master_end()
{
   hrt_stop(&hr_tm);
   if(list_size(factors)>0){
      printf("Factors: ");
      print_list(factors);
      printf("Elapsed time: %f seconds\n", hrt_elapsed_time(&hr_tm));    
   }
   destroy_list_nodes(factors);
   destroy_list(&factors);
}

int has_process_up(char *process, int nprocs)
{
   int i;
   for( i=0; i<nprocs; i++){
      if(process[i]) return 1;
   }
   return 0;
}

void calculate_interval(int rank, mpz_t n_interval, mpz_t n_start, mpz_t n_end )
{
   if( MASTER(rank) ) {
      // A thread de id = 0 assume o intervalo [3, n_interval].
      mpz_set_ui(n_start, 3);
      if(mpz_cmp(n_start, n_interval) >= 0)
         mpz_set(n_end, n_start);
      else
         mpz_set(n_end, n_interval);
   } else {
      // n_start = n_interval * thread.id + 1
      mpz_mul_ui(n_start, n_interval, rank);
      mpz_add_ui(n_start, n_start, 1);

      // n_end = n_interval * (thread.id + 1)
      mpz_mul_ui(n_end, n_interval, (rank + 1));
  }
}

void random_number(mpz_t n, int numlen)
{
    char numstr[numlen+1];
    int i;

    srand( time(NULL) );
    
    numstr[0] = '1'+(rand()%9);
    for(i=1; i<numlen; i++) {
        numstr[i] = '0'+(rand()%10);
    }
    numstr[numlen] = 0;

    mpz_set_str(n, numstr, 10);
}

/*
 * This procedure prints all elements of the given list.
 */
void print_list(list_t *list)
{
    node_t *node = list->begin;
    while(node!=NULL) {
        gmp_printf("%Zd", NODEGET( mpz_t, node ));
        node = node->next;
        if(node)
            putchar(' ');
        else putchar('\n');
    }
}

