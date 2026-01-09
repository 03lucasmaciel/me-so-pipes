/*
 * ============================================================================
 * CLIENTE - Projeto SO 25/26
 * ============================================================================
 * 
 * OBJETIVO:
 * Este programa envia comandos para o servidor através de um FIFO (named pipe).
 * 
 * COMO FUNCIONA:
 * 1. Recebe os comandos como argumentos da linha de comando
 * 2. Junta todos os comandos numa única mensagem, separados por ';'
 * 3. Envia a mensagem para o servidor através do FIFO
 * 4. Fecha a conexão
 * 
 * EXEMPLO DE USO:
 *   ./client "ls -la" "pwd" "date"
 *   
 *   Isto envia para o servidor: "ls -la;pwd;date"
 * 
 * ============================================================================
 */

#include <stdlib.h>     // exit(), EXIT_FAILURE
#include <unistd.h>     // write(), close(), STDOUT_FILENO
#include <fcntl.h>      // open(), O_WRONLY
#include <sys/stat.h>   // permissões de ficheiros
#include <string.h>     // strlen(), strcat()
#include <errno.h>      // errno

/*
 * ============================================================================
 * FUNÇÕES AUXILIARES PARA I/O SEM USAR STDIO.H
 * ============================================================================
 */

/*
 * Escreve uma string no stdout
 */
void print_str(const char *str) {
    write(STDOUT_FILENO, str, strlen(str));
}

/*
 * Escreve uma string no stderr
 */
void print_err(const char *str) {
    write(STDERR_FILENO, str, strlen(str));
}

/*
 * Converte um inteiro para string e escreve-o
 */
int print_int(int fd, int num) {
    char buffer[32];
    int i = 0;
    int is_negative = 0;
    
    if (num == 0) {
        write(fd, "0", 1);
        return 1;
    }
    
    if (num < 0) {
        is_negative = 1;
        num = -num;
    }
    
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    if (is_negative) {
        buffer[i++] = '-';
    }
    
    for (int j = i - 1; j >= 0; j--) {
        write(fd, &buffer[j], 1);
    }
    
    return i;
}

/*
 * Substitui perror()
 */
void print_error(const char *msg) {
    print_err("[CLIENT] ");
    print_err(msg);
    print_err(": ");
    
    switch (errno) {
        case EACCES:
            print_err("Permission denied");
            break;
        case EEXIST:
            print_err("File exists");
            break;
        case ENOENT:
            print_err("No such file or directory");
            break;
        case ENOMEM:
            print_err("Out of memory");
            break;
        default:
            print_err("Error code ");
            print_int(STDERR_FILENO, errno);
            break;
    }
    print_err("\n");
}

/* 
 * Caminho do FIFO - tem de ser igual no cliente e no servidor
 * O FIFO é criado em /tmp porque é uma pasta acessível a todos
 */
#define FIFO_PATH "/tmp/exec_fifo"

/* 
 * Tamanho máximo da mensagem que podemos enviar
 * 4096 bytes é suficiente para vários comandos
 */
#define MAX_MESSAGE 4096


/*
 * ============================================================================
 * FUNÇÃO PRINCIPAL (main)
 * ============================================================================
 * 
 * PARÂMETROS:
 *   - argc: número de argumentos (incluindo o nome do programa)
 *   - argv: array com os argumentos
 *     - argv[0] = nome do programa ("./client")
 *     - argv[1] = primeiro comando
 *     - argv[2] = segundo comando
 *     - etc...
 * 
 * RETORNO:
 *   - 0 se correu tudo bem
 *   - EXIT_FAILURE se houve algum erro
 */
int main(int argc, char *argv[]) {
    int fd;                      // Descritor do ficheiro FIFO
    char message[MAX_MESSAGE];   // Buffer para construir a mensagem
    
    /*
     * ========================================================================
     * PASSO 1: Verificar se o utilizador passou comandos
     * ========================================================================
     * Se argc < 2, significa que só temos argv[0] (o nome do programa)
     * e o utilizador não passou nenhum comando
     */
    if (argc < 2) {
        write(STDOUT_FILENO, "Uso: ./client \"cmd1 args\" \"cmd2 args\" ...\n", 42);
        write(STDOUT_FILENO, "Exemplo: ./client \"ls -la\" \"pwd\" \"date\"\n", 40);
        exit(EXIT_FAILURE);
    }

    /*
     * ========================================================================
     * PASSO 2: Construir a mensagem com todos os comandos
     * ========================================================================
     * 
     * PROTOCOLO DEFINIDO:
     * - Os comandos são separados por ';' (ponto e vírgula)
     * - Exemplo: "ls -la;pwd;date"
     * 
     * Este ciclo percorre todos os argumentos (comandos) e junta-os
     * numa única string, separados por ';'
     */
    message[0] = '\0';  // Inicializa a mensagem vazia
    
    for (int i = 1; i < argc; i++) {
        
        // Verifica se ainda há espaço na mensagem
        if (strlen(message) + strlen(argv[i]) + 2 > MAX_MESSAGE) {
            print_err("[CLIENT] Erro: mensagem demasiado longa\n");
            exit(EXIT_FAILURE);
        }
        
        // Adiciona o comando à mensagem
        strcat(message, argv[i]);
        
        // Adiciona ';' entre os comandos (mas não depois do último)
        if (i < argc - 1) {
            strcat(message, ";");
        }
    }

    /*
     * ========================================================================
     * PASSO 3: Abrir o FIFO para escrita
     * ========================================================================
     * 
     * open() com O_WRONLY abre o FIFO apenas para escrita
     * 
     * NOTA IMPORTANTE:
     * Esta chamada BLOQUEIA até que o servidor abra o FIFO para leitura!
     * Por isso, o servidor tem de estar a correr primeiro.
     */
    fd = open(FIFO_PATH, O_WRONLY);
    if (fd == -1) {
        print_error("open");  // Mostra o erro (ex: "No such file or directory")
        exit(EXIT_FAILURE);
    }

    /*
     * ========================================================================
     * PASSO 4: Enviar a mensagem para o servidor
     * ========================================================================
     * 
     * write() envia os dados através do FIFO
     * - fd: descritor do FIFO
     * - message: dados a enviar
     * - strlen(message): número de bytes a enviar
     * 
     * IMPORTANTE: Enviamos tudo num único write() conforme o enunciado pede
     */
    ssize_t written = write(fd, message, strlen(message));
    if (written == -1) {
        print_error("write");
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    /*
     * ========================================================================
     * PASSO 5: Mostrar confirmação ao utilizador
     * ========================================================================
     */
    print_str("[CLIENT] Enviados ");
    print_int(STDOUT_FILENO, argc - 1);
    print_str(" comando(s):\n");
    for (int i = 1; i < argc; i++) {
        print_str("  ");
        print_int(STDOUT_FILENO, i);
        print_str(": ");
        print_str(argv[i]);
        print_str("\n");
    }

    /*
     * ========================================================================
     * PASSO 6: Fechar o FIFO
     * ========================================================================
     * 
     * Fechar o FIFO é MUITO IMPORTANTE porque:
     * - Sinaliza ao servidor que terminámos de enviar (EOF)
     * - Liberta os recursos do sistema
     */
    close(fd);
    
    return 0;
}
