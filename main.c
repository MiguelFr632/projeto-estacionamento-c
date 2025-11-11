#include <stdio.h>
#include <string.h>
#include "estacionamento.h"

static void remover_quebra_de_linha(char *s) {
    if (!s) return;
    s[strcspn(s, "\r\n")] = '\0';
}

int main(void) {
    for (;;) { // Loop infinito até o usuário decidir sair.
        printf("\n=== BEM VINDO AO CONTROLE DE ESTACIONAMENTO ===");
        printf("\n1) Cadastrar entrada");
        printf("\n2) Cadastrar saída");
        printf("\n3) Consultar veículo");
        printf("\n\nEscolha uma opção (1 a 3 ou 0 para sair) e pressione ENTER:");

        char optbuf[16];
        if (!fgets(optbuf, sizeof(optbuf), stdin)) {
            printf("\nEncerrando...");
            break;
        }
        remover_quebra_de_linha(optbuf);

        if (strlen(optbuf) != 1 || optbuf[0] < '0' || optbuf[0] > '3') {
            printf("Opção inválida. Voltando ao início.");
            continue;
        }

        switch (optbuf[0]) {
            case '1':
                cadastrar_entrada();
                break;
            case '2':
                cadastrar_saida();
                break;
            case '3':
                consultar_veiculo();
                break;
            case '0':
                printf("\nEncerrando...\n");
                return 0;
        }
    }

    return 0;
}
