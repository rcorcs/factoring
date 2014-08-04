/*
 * factoring
 * 
 * compile: gcc main.c ./datatypes/*.c ./time/*.c -o factoring -lgmp -lrt
 * run: ./factoring (-n <number> | -r <num_digits>)
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <gmp.h> //Big Numbers

#include "datatypes/list.h"
#include "time/hr_time.h"


__inline int prime(mpz_t n);
void test_result(list_t *list);
void print_list(list_t *list);
void get_divisor(mpz_t n, mpz_t d);
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
    node_t *node;
    list_t *temp = create_list();
    list_t *factors = create_list();
    char *nstr;
    int nlen;
    mpz_t number;
    mpz_t divisor;
    mpz_t quotient;

    mpz_init(number);
    mpz_init(divisor);
    mpz_init(quotient);
    
    if(argc > 2){
       if(!strcmp(argv[1], "-r")){
          random_number(number, atoi(argv[2]));
       }else if(!strcmp(argv[1], "-n")){
          mpz_set_str(number, argv[2], 10);
       }else {
          printf("usage:\n factoring -n <number>\n factoring -r <num_digits>\n");
          return 0;
       }
    }else {
       printf("usage:\n factoring -n <number>\n factoring -r <num_digits>\n");
       return 0;
    }
    nstr = mpz_get_str(NULL, 10, number);
    nlen = strlen(nstr);
    
    //gmp_printf("Number: (%d) %Zd\nplease wait...\n", nlen, number);
    gmp_printf("Number: (%d) %Zd\n", nlen, number);

    hrt_start(&hr_tm);
    do {
        node = alloc_node(sizeof(mpz_t));
        mpz_init(NODEGET( mpz_t, node ));
        get_divisor(number, divisor);
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

    //test_result(factors);

    destroy_list_nodes(factors);
    destroy_list(&temp);
    destroy_list(&factors);

    return 0;
}

void get_divisor(mpz_t n, mpz_t d)
{
    mpz_t q, r, temp;
    mpz_t n_sqrt;
    mpz_init(r); //remainder


    mpz_mod_ui(r, n, 2);
    if( (mpz_cmp_ui(n,2) > 0) && mpz_sgn(r)==0 ) {
        mpz_set_ui(d, 2);
        return;
    } else {
        mpz_init(n_sqrt);
        mpz_init(temp);
        mpz_init(q);

        mpz_sqrt(n_sqrt, n);

        for( mpz_set_ui(q, 3); mpz_cmp(n_sqrt,q) >= 0; mpz_add_ui(temp, q, 2), mpz_set(q, temp) ) {
            mpz_mod(r, n, q);
            if( !mpz_sgn(r) ) {
                mpz_set(d, q);
                return;
            }
        }
    }
    mpz_set_ui(d, 0);
}

/*
 * This function tests if a given number is prime.
 * It returns 1 if it is prime and 0 otherwise.
 */
__inline int prime(mpz_t n)
{
    mpz_t d;
    mpz_init(d);
    get_divisor(n, d);
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
void test_result(list_t *list)
{
    node_t *node = list->begin;
    printf("RESULT TEST:\n");
    while(node!=NULL) {
        if( prime( NODEGET( mpz_t, node ) ) )
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
