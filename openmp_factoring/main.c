/*
 * openmp_factoring
 * 
 * compile: gcc main.c ./datatypes/*.c ./time/*.c -o openmp_factoring -lgmp -lrt -fopenmp
 * run: ./openmp_factoring (-n <number> | -r <num_digits>)  [-t <num_threads>]
*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <gmp.h> //Big Numbers

#include "datatypes/list.h"
#include "time/hr_time.h"

//#define DEBUG

__inline int prime(mpz_t n, int num_threads);
void test_result(list_t *list, int num_threads);
void print_list(list_t *list);
void get_divisor(mpz_t n, mpz_t d, int num_threads);
void random_number(mpz_t n, int numlen);
/*
 * This program finds all prime factors of a certain number and prints them on the screen.
 */
/*
 * "Obscure Logic":
 *     It keeps a temporary list of the divisors that has been found until now and
 *     check the divisors of the current number, if the current number is prime then it removes
 *     one of the divisors that is in the temporary list and make it the current number.
 *     When it finds a divisor for the current number, then the divisor becomes the current number,
 *     and the quotient is saved into the temporary list.
 *     Note: If the number is already prime, then the resulting factors list contains the number itself.
 */
int main(int argc, char **argv)
{
    hr_timer_t hr_tm;
    int num_threads = 2;
    node_t *node;
    list_t *temp = create_list();
    list_t *factors = create_list();
    mpz_t number;
    mpz_t divisor;
    mpz_t quotient;
    char *nstr;
    int nlen;
    mpz_init(number);
    mpz_init(divisor);
    mpz_init(quotient);

    if(argc > 2){
       if(!strcmp(argv[1], "-r")){
          random_number(number, atoi(argv[2]));
       }else if(!strcmp(argv[1], "-n")){
          mpz_set_str(number, argv[2], 10);
       }else {
          printf("usage:\n factoring -n <number> [-t <num_threads>]\n factoring -r <num_digits> [-t <num_threads>]\n");
          return 0;
       }
       if(argc > 4){
          if(!strcmp(argv[3], "-t")){
             num_threads = atoi(argv[4]);
          }
       }
    }else {
       printf("usage:\n factoring -n <number> [-t <num_threads>]\n factoring -r <num_digits> [-t <num_threads>]\n");
       return 0;
    }
    nstr = mpz_get_str(NULL, 10, number);
    nlen = strlen(nstr);
    gmp_printf("Number: (%d) %Zd\n", nlen, number);

    hrt_start(&hr_tm);
    do {
        node = alloc_node(sizeof(mpz_t));
        mpz_init(NODEGET( mpz_t, node ));
        get_divisor(number, divisor, num_threads);
        if( mpz_sgn(divisor) ) {
            mpz_cdiv_q(quotient,number,divisor);
            mpz_set(NODEGET( mpz_t, node ), quotient);
            list_push_back(temp, node);
            mpz_set(number,divisor);
            
        } else {
            //gmp_printf("factor: %Zd\n", number);
            mpz_set(NODEGET( mpz_t, node ), number);
            list_push_back(factors, node);
            node = list_pop_front(temp);
            if(node) {
                mpz_set(number, NODEGET( mpz_t, node ));
                destroy_node(&node);
            } else {
                mpz_set_ui(number, 0);
            }

        }
    } while( mpz_sgn(number) );
    hrt_stop(&hr_tm);

    printf("Factors: ");
    print_list(factors);

    printf("Elapsed time: %f seconds\n", hrt_elapsed_time(&hr_tm));

    //test_result(factors, num_threads);

    destroy_list_nodes(factors);
    destroy_list(&temp);
    destroy_list(&factors);

    return 0;
}

/**
 * Implementação paralela da função get_divisor() utilizando o modelo de memória compartilhada.
 *
 * Diferentemente da versão sequencial, na qual se buscava o menor divisor possível de um
 * número N, dentro de um intervalo válido especificado, devido à natureza paralela desta
 * implementação, busca-se e retorna-se um divisor qualquer, válido dentro do intervalo, para N,
 * sendo que este divisor pode ser encontrado por qualquer uma das threads que processam
 * intervalos distintos. A única exceção é para valores de N pares, para os quais
 * o divisor 2 sempre será retornado e representa o menor divisor válido para tais valores de N.
 *
 * Os limites para a busca são mantidos no intervalo geral [2, sqrt(N)].
 *
 * @param n
 *    número para o qual um divisor será buscado.
 * @param d
 *    variável que receberá o valor do divisor válido, se este existir.
 *    Caso nenhum divisor válido seja encontrado, este parâmetro receberá o valor "0".
 */
void get_divisor(mpz_t n, mpz_t d, int num_threads)
{
    int found_divisor = 0;
    mpz_t r;
    mpz_t n_sqrt;
    mpz_t n_interval;
    mpz_init(r); //remainder
    
    if( mpz_cmp_ui(n, 3)<=0){
       mpz_set_ui(d,0);
       return;
    }
    // ------------------------ Trecho sequencial -----------------------------
    //
    // Verifica caso base: se o número for divisível por 2, não há necessidade
    // de divisão de intervalos entre threads.
    mpz_mod_ui(r, n, 2);

    if( mpz_sgn(r)==0 ) {
        found_divisor = 1;
        mpz_set_ui(d, 2);
        return;
    } else {

        // ------------------------ Trecho paralelo -----------------------------

        // Apenas para fins de depuração.num_thread := get_num_threads()
        #ifdef DEBUG
           gmp_printf("\nAlocando threads para encontrar um divisor para: %Zd\n\n", n);
        #endif

        mpz_init(n_sqrt);
        mpz_init(n_interval);

        // Define limite superior para busca => sqrt(N).
        mpz_sqrt(n_sqrt, n);

        // Define número de iterações que cada thread deve realizar =>
        // n_interval = sqrt(N) / numero de threads.
        mpz_cdiv_q_ui(n_interval, n_sqrt, num_threads);
        #ifdef DEBUG
           gmp_printf("\nTamanho do intervalo calculado: %Zd\n\n", n_interval);
        #endif


        // Calcula o resto da divisão de N pelo número de threads. Como o resto da divisão
        // será no máximo NUM_THREADS - 1 e este valor pode ser representado em 4 bytes,
        // o uso de uma variável do tipo inteiro é mais indicado para essa operação.
        //
        // Visto que a divisão do número N alvo em intervalos pode ser inexata, inevitavelmente
        // algumas threads terão um intervalo de busca ligeiramente maior que as demais. Essa
        // variável então representa o número de divisores que não estão alocados
        // de forma uniforme nos intervalos de busca das threads ou, de forma equivalente,
        // o número de threads que terão seu limite superior de busca maior que as outras.
        //
        // Para prover um melhor balanceamento de carga, no processo de divisão de intervalos,
        // as threads com intervalos que compreendem valores menores deverão incrementar seu
        // respectivo limite superior de busca a fim de alocar um desses divisores não alocados.
        //
        // A justificativa é que, dada a representação de valores inteiros grandes pela biblioteca
        // GMP, threads que processam intervalos com valores relativamente pequenos tendem a
        // terminar tal processamento em menor tempo comparado àquelas que processam valores
        // maiores. Logo, o ideal é que essas threads com menor carga de trabalho assumam o maior
        // número de valores a serem analisados, caso seja necessário.

        //unsigned int numeroDeDivisoresNaoAlocados = mpz_fdiv_ui (n_sqrt, num_threads);

        omp_set_num_threads(num_threads);

        // Marca região paralela no código: TODAS as threads alocadas irão executar o bloco
        // de código abaixo. A divisão de intervalos será feita por cada thread dentro da região
        // parelela.
        //
        // Esta abordagem permite o ajuste dos intervalos de forma dinâmica e em função
        // do número de threads disponíveis, sendo, portanto, mais escalável que a abordagem
        // utilizando sections.
        //
        // Se tivéssemos adotado o uso de sections, estaríamos explicitando o número máximo de
        // threads que nossa aplicação suportaria, o que deixa de ser interessante para
        // fins de comparação. E esta é outra vantagem latente desta abordagem utilizada:
        // devido à possibilidade de variar o número de threads, inclusive em tempo de execução,
        // a tarefa de testes se torna mais robusta, bastanto apenas ajustar os parâmetros
        // necessários ao executar a aplicação.
        //
        // As variáveis found_divisor, numeroDeDivisoresNaoAlocados e n_interval devem ser
        // COMPARTILHADAS entre as diversas threads para que estas possam definir seus
        // respectivos intervalos de busca.
        //#pragma omp parallel shared(found_divisor, n_interval, numeroDeDivisoresNaoAlocados)
        #pragma omp parallel shared(found_divisor, n_interval)
        {
            // Variáveis privadas a cada thread por construção.
            mpz_t quotient, remainder, temp;
            mpz_t n_start;
            mpz_t n_end;
            mpz_init(n_start);
            mpz_init(n_end);
            mpz_init(quotient);
            mpz_init(remainder);
            mpz_init(temp);

            // Retorna identificador da thread corrente.
            int threadId = omp_get_thread_num();

            // Define divisor inicial de busca para a thread corrente:

            if(threadId == 0) {

                // A thread de id = 0 assume o intervalo [3, n_interval].
                mpz_set_ui(n_start, 3);
                if(mpz_cmp(n_start, n_interval) >= 0)
                   mpz_set(n_end, n_start);
                else
                   mpz_set(n_end, n_interval);

            } else {
                mpz_t r0;
                mpz_init(r0); //remainder
                // As demais threads assumem o intervalo definido por:
                // Inicial:  n_interval * thread.id + 1
                // Final:    n_interval * (thread.id + 1)
                //
                // Esse processo pode definir um número par como primeiro divisor
                // a ser testado, no entanto, como o número de threads alocadas tende a ser
                // pequeno, o teste desnecessário de um divisor par (que deveria ser detectado
                // no trecho sequencial deste algoritmo) provavelmente não causará perdas
                // significativas de desempenho.
                //
                // Uma otimização poderia ser feita, se julgado necessário, ao verificar se o
                // intervalo inicial é divisível por 2 e incrementá-lo caso afirmativo.
                // A possível otimização reside no fato de que dividir um número por 2, dada
                // a implementação da biblioteca GMP, tende a ser mais rápido que realizar
                // a divisão entre dois números quaisquer relativamente grandes.

                // n_start = n_interval * thread.id + 1
                mpz_mul_ui(n_start, n_interval, threadId);
                mpz_mod_ui(r0, n_start, 2);
                if( mpz_sgn(r0)==0 ) {
                   mpz_add_ui(n_start, n_start, 1);
                }

                // n_end = n_interval * (thread.id + 1)
                if(mpz_cmp_ui(n_start, 1)>0){
                   mpz_mul_ui(n_end, n_interval, (threadId + 1));
                }else {
                   mpz_set_ui(n_end, 0);
                }
            }

            // Correção dos intervalos:

            // Verifica se a thread corrente deve assumir um divisor não alocado,
            // isto é, verifica-se se o limite superior do intervalo para esta thread
            // deve ser incrementado.
            //if(threadId < numeroDeDivisoresNaoAlocados)
            //{
            //    mpz_add_ui(n_end, n_end, 1);
            //}
            //else if(numeroDeDivisoresNaoAlocados > 0)
            //{
                // Do contrário, caso existam divisores não alocados,
                // significa que a thread que compreende o intervalo imediatamente
                // anterior ao corrente considerou um divisor não alocado e, portanto,
                // o valor inicial de busca, nesta thread, deve ser incrementado para
                // que tal divisor não alocado possivelmente testado (se for ímpar) anteriormente
                // não seja retestado aqui.

            //    mpz_add_ui(n_start, n_start, 1);
            //}


            // Apenas para fins de depuração.
            #ifdef DEBUG
              gmp_printf("Thread: %d\nInicial: %Zd\nFinal: %Zd\n\n", threadId, n_start, n_end);
            #endif


            // Cada thread procura por um divisor de N em seu respectivo intervalo, alterando
            // o valor da flag de controle "found_divisor" ao encontrar tal divisor. Encontrado um
            // divisor qualquer, todas as threads interropem seus loops.
            for(mpz_set(quotient, n_start); (!found_divisor) && mpz_cmp(n_end,quotient) >= 0; mpz_add_ui(quotient, quotient, 2) ) {

                mpz_mod(remainder, n, quotient);

                if( !mpz_sgn(remainder) ) {

                    // Garante que apenas UMA thread executará o bloco delimitado abaixo por vez.
                    #pragma omp critical
                    {
                        // Só atualiza o divisor encontrado "d" se nenhum outro tiver sido
                        // encontrado por outra thread que entrou nesta seção crítica anteriormente.
                        if(found_divisor == 0) {

                            // Retorna divisor encontrado e altera variável de controle
                            // para interromper o processo de busca nas threads alocadas.
                            mpz_set(d, quotient);
                            found_divisor = 1;

                           // Apenas para fins de depuração.
                           #ifdef DEBUG
                             gmp_printf("Thread: %d encontrou o divisor: %Zd\n\n", threadId, quotient);
                           #endif
                        }
                    }
                }
            }
        }
    }

    // Se nenhum divisor tiver sido encontrado por alguma thread, então
    // assume-se que N não possui divisores entre 3 e sqrt(N).
    if(!found_divisor) mpz_set_ui(d, 0);
}

/*
 * This function tests if a given number is prime.
 * It returns 1 if it is prime and 0 otherwise.
 */
__inline int prime(mpz_t n, int num_threads)
{
    mpz_t d;
    mpz_init(d);
    get_divisor(n, d, num_threads);
    return( !mpz_sgn(d) );
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

/*
 * This procedure tests if all results found are actually primes
 * and prints the results on the screen.
 */
void test_result(list_t *list, int num_threads)
{
    node_t *node = list->begin;
    printf("RESULT TEST:\n");
    while(node!=NULL) {
        if( prime( NODEGET( mpz_t, node ), num_threads) )
            gmp_printf("%Zd is prime\n", NODEGET( mpz_t, node ));
        else gmp_printf("%Zd is NOT prime\n", NODEGET( mpz_t, node ) );

        node = node->next;
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

