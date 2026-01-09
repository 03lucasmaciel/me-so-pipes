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

#include <stdio.h>      // printf()
#include <stdlib.h>     // exit(), EXIT_FAILURE
#include <unistd.h>     // write(), close(), STDOUT_FILENO
#include <fcntl.h>      // open(), O_WRONLY
#include <sys/stat.h>   // permissões de ficheiros
#include <string.h>     // strlen(), strcat()

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
            write(STDERR_FILENO, "[CLIENT] Erro: mensagem demasiado longa\n", 40);
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
        perror("open");  // Mostra o erro (ex: "No such file or directory")
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
        perror("write");
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    /*
     * ========================================================================
     * PASSO 5: Mostrar confirmação ao utilizador
     * ========================================================================
     */
    printf("[CLIENT] Enviados %d comando(s):\n", argc - 1);
    for (int i = 1; i < argc; i++) {
        printf("  %d: %s\n", i, argv[i]);
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
