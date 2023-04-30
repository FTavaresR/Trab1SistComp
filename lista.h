// lista.h

#ifndef LISTA_H
#define LISTA_H

// Estrutura da lista
typedef struct client_data {
    int sk;
    struct sockaddr_in *client_addr;
    enum Estado { OCIOSO, TRABALHANDO } estado;
}client_data;

typedef struct No {
    client_data* data;
    struct No* proximo;
} No;

// Funções para manipulação da lista
No* criar_lista();
void adicionar_elemento(No** cabeca, client_data* valor);
void remover_elemento(No** cabeca, client_data* valor);
void print_lista(No* cabeca);
No* busca_worker(No* cabeca);
#endif