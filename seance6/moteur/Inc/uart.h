#ifndef UART_H
#define UART_H

#include <stdint.h>

/*
 * Initialise USART2 :
 * - TX sur PA2 ;
 * - 115200 bauds ;
 * - 8 bits ;
 * - aucune parité ;
 * - 1 bit de stop.
 */
void uart_init(void);

/* Envoie un caractère. */
void uart_send_char(char caractere);

/* Envoie une chaîne terminée par '\0'. */
void uart_send_string(const char *texte);

/* Envoie un entier non signé en écriture décimale. */
void uart_send_uint(uint32_t nombre);

#endif /* UART_H */