#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "estacionamento.h"

#define DB_FILE "placas.txt"
#define LINE_MAX 128

// Remove o caractere 'enter' do final de uma string.
static void remover_quebra_de_linha(char *s) {
    if (!s) return;
    s[strcspn(s, "\r\n")] = '\0';
}

// Convertendo todos os caracteres da string para maiúsculas.
static void converter_para_maiuscula(char *s) {
    if (!s) return;
    for (; *s; ++s) *s = (char) toupper((unsigned char)*s);
}

// Verifica se uma string está no formato de placa antiga (AAA9999) ou Mercosul (AAA9A99).
static int eh_placa_valida(const char *p) {
    if (!p) return 0;
    size_t n = strlen(p);
    if (n != 7) return 0;

    if (isalpha((unsigned char)p[0]) && isalpha((unsigned char)p[1]) && isalpha((unsigned char)p[2]) &&
        isdigit((unsigned char)p[3]) && isdigit((unsigned char)p[4]) &&
        isdigit((unsigned char)p[5]) && isdigit((unsigned char)p[6])) {
        return 1;
    }

    if (isalpha((unsigned char)p[0]) && isalpha((unsigned char)p[1]) && isalpha((unsigned char)p[2]) &&
        isdigit((unsigned char)p[3]) &&
        isalpha((unsigned char)p[4]) &&
        isdigit((unsigned char)p[5]) && isdigit((unsigned char)p[6])) {
        return 1;
    }

    return 0;
}

// Pega a data e hora atuais do sistema e formata como "dd/mm/aaaa HH:MM".
static void obter_data_hora_atual(char *buf, size_t bufsize) {
    time_t t = time(NULL);
    struct tm tmv;
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS) || defined(__unix__) || defined(__APPLE__)
    localtime_r(&t, &tmv);
#else
    struct tm *tmp = localtime(&t);
    if (tmp) tmv = *tmp;
#endif
    strftime(buf, bufsize, "%d/%m/%Y %H:%M", &tmv);
}


// Procura pela placa que o usuário informou, no arquivo "placas.txt".
static int encontrar_placa(const char *nome_arquivo, const char *placa, char *datetime_out, size_t outsz) {
    FILE *f = fopen(nome_arquivo, "r");
    if (!f) return 0; 

    char line[LINE_MAX];
    int found = 0;

    while (fgets(line, sizeof(line), f)) {
        remover_quebra_de_linha(line);
        char *comma = strchr(line, ',');
        
        if (!comma) continue;
        
        char *placa_no_arquivo = comma + 1;
        
        if (strcmp(placa_no_arquivo, placa) == 0) {
            if (datetime_out && outsz > 0) {
                size_t len = (size_t)(comma - line);
                
                if (len >= outsz) len = outsz - 1;
                memcpy(datetime_out, line, len);
                datetime_out[len] = '\0';
            }
            found = 1;
            break;
        }
    }

    fclose(f);
    return found;
}


// Adiciona um novo registro no final do arquivo "placas.txt".
static int adicionar_placa(const char *nome_arquivo, const char *placa) {
    FILE *f = fopen(nome_arquivo, "a");
    if (!f) return 0;

    char stamp[32];
    
    obter_data_hora_atual(stamp, sizeof(stamp));
    
    int ok = fprintf(f, "%s,%s\n", stamp, placa) > 0;

    fclose(f);
    return ok;
}


// Remove a linha do arquivo "placas.txt" que está a placa informada para remoção.
static int remover_placa(const char *nome_arquivo, const char *placa) {
    FILE *fin = fopen(nome_arquivo, "r");
    
    if (!fin) return 0;

    char nome_temporario[] = "placas.tmp";
    FILE *fout = fopen(nome_temporario, "w");
    
    if (!fout) {
        fclose(fin);
        return 0;
    }

    char line[LINE_MAX];
    int removido = 0;

    while (fgets(line, sizeof(line), fin)) {
        char copy[LINE_MAX];
        
        strncpy(copy, line, sizeof(copy));
        
        copy[sizeof(copy)-1] = '\0';
        
        remover_quebra_de_linha(copy);

        char *comma = strchr(copy, ',');
        
        if (!comma) {
            fputs(line, fout);
            continue;
        }
        
        char *placa_no_arquivo = comma + 1;

        if (strcmp(placa_no_arquivo, placa) == 0) {
            removido++;
            continue; 
        } else {
            fputs(line, fout);
        }
    }

    fclose(fin);
    fclose(fout);


    if (remove(nome_arquivo) != 0) {
        remove(nome_temporario);
        return 0;
    }
    
    if (rename(nome_temporario, nome_arquivo) != 0) {
        return 0;
    }

    return removido;
}


// Mostra uma mensagem e lê uma linha de texto digitada pelo usuário.
static int ler_linha_de_entrada(const char *prompt, char *buf, size_t sz) {
    fputs(prompt, stdout);
    
    fflush(stdout);
    
    if (!fgets(buf, (int)sz, stdin)) return 0;
    
    remover_quebra_de_linha(buf);
    
    return 1;
}

// Responsável pela lógica de registrar a entrada de um novo veículo.
void cadastrar_entrada(void) {
    char placa[64];

    if (!ler_linha_de_entrada("Digite a placa (formato AAA9999 ou AAA9A99): ", placa, sizeof(placa))) {
        printf("Entrada inválida. Voltando ao menu.");
        return;
    }
    converter_para_maiuscula(placa);

    if (!eh_placa_valida(placa)) {
        printf("Placa inexistente! Voltando ao menu.");
        return;
    }

    char dt[32];
    
    if (encontrar_placa(DB_FILE, placa, dt, sizeof(dt))) {
        printf("Veículo já está no estacionamento desde %s.\n", dt);
        return;
    }

    if (adicionar_placa(DB_FILE, placa)) {
        printf("Entrada cadastrada com sucesso!");
    } else {
        printf("Falha ao gravar no banco de dados.");
    }
}


// Responsável pela lógica de registrar a SAÍDA de um veículo.
void cadastrar_saida(void) {
    char placa[64];

    if (!ler_linha_de_entrada("Digite a placa para saída: ", placa, sizeof(placa))) {
        printf("Entrada inválida. Voltando ao menu.");
        return;
    }
    converter_para_maiuscula(placa);

    if (!eh_placa_valida(placa)) {
        printf("Placa inexistente. Voltando ao menu.");
        return;
    }
    

    int removido = remover_placa(DB_FILE, placa);
    
    if (removido > 0) {
        printf("Saída registrada. (%d registro%s removido%s)\n",
               removido, removido == 1 ? "" : "s", removido == 1 ? "" : "s");
    } else {
        printf("Placa não encontrada no banco de dados.");
    }
}


// Responsável pela lógica de procurar por um veículo no estacionamento (placas.txt).
void consultar_veiculo(void) {
    char placa[64];
    
    if (!ler_linha_de_entrada("Digite a placa para consulta: ", placa, sizeof(placa))) {
        printf("Entrada inválida. Voltando ao menu.");
        return;
    }
    converter_para_maiuscula(placa);

    if (!eh_placa_valida(placa)) {
        printf("Placa inexistente (formato inválido). Voltando ao menu.");
        return;
    }

    char dt[32];
    
    if (encontrar_placa(DB_FILE, placa, dt, sizeof(dt))) {
        printf("Veículo no estacionamento desde %s.\n", dt);
    } else {
        printf("Veículo NÃO está no estacionamento.");
    }
}