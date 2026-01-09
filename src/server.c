/*
 * ============================================================================
 * SERVIDOR - Projeto SO 25/26
 * ============================================================================
 * 
 * OBJETIVO:
 * Este programa recebe comandos do cliente através de um FIFO, executa-os
 * e regista os resultados num ficheiro de log.
 * 
 * COMO FUNCIONA:
 * 1. Cria o FIFO (named pipe) se não existir
 * 2. Fica à espera de mensagens do cliente
 * 3. Quando recebe uma mensagem, separa os comandos (por ';')
 * 4. Para cada comando, cria um processo filho que o executa
 * 5. Espera que todos os filhos terminem
 * 6. Regista os resultados no ficheiro de log
 * 7. Volta ao passo 2
 * 
 * EXEMPLO:
 *   Cliente envia: "ls -la;pwd;date"
 *   Servidor:
 *     - Separa em 3 comandos: "ls -la", "pwd", "date"
 *     - Cria 3 processos filho
 *     - Cada filho executa o seu comando
 *     - Regista os 3 resultados no log
 * 
 * ============================================================================
 */

#include <stdio.h>      // printf(), perror()
#include <stdlib.h>     // exit(), EXIT_FAILURE
#include <unistd.h>     // read(), write(), close(), fork(), _exit()
#include <fcntl.h>      // open(), O_RDONLY, O_WRONLY, O_CREAT, O_APPEND
#include <sys/stat.h>   // mkdir(), mkfifo()
#include <string.h>     // strlen(), strtok_r(), strdup(), strncpy()
#include <errno.h>      // errno, EEXIST
#include <sys/wait.h>   // waitpid(), WIFEXITED(), WEXITSTATUS()

/* 
 * Caminho do FIFO - tem de ser igual no cliente e no servidor
 */
#define FIFO_PATH "/tmp/exec_fifo"

/* 
 * Caminho do ficheiro de log onde guardamos os resultados
 */
#define LOG_FILE "logs/server.log"

/* 
 * Tamanho máximo do buffer de leitura
 */
#define MAX_BUFFER 4096


/*
 * ============================================================================
 * FUNÇÃO: append_log
 * ============================================================================
 * 
 * OBJETIVO:
 * Escreve uma linha no ficheiro de log.
 * 
 * PARÂMETROS:
 *   - line: texto a escrever no log
 * 
 * COMO FUNCIONA:
 * 1. Abre o ficheiro de log (cria se não existir)
 * 2. Escreve a linha no final do ficheiro (append)
 * 3. Fecha o ficheiro
 * 
 * NOTA: Abrimos e fechamos o ficheiro em cada escrita para garantir
 * que os dados são guardados mesmo se o servidor crashar.
 */
void append_log(const char *line) {
    /*
     * Abre o ficheiro de log:
     * - O_WRONLY: apenas para escrita
     * - O_CREAT: cria o ficheiro se não existir
     * - O_APPEND: escreve sempre no final do ficheiro
     * - 0644: permissões (rw-r--r--)
     */
    int log_fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd == -1) {
        perror("[SERVER] Erro ao abrir o ficheiro de log");
        return;
    }

    // Escreve a linha no ficheiro
    ssize_t written = write(log_fd, line, strlen(line));
    if (written == -1) {
        perror("[SERVER] Erro ao escrever no ficheiro de log");
    }

    // Fecha o ficheiro
    close(log_fd);
}


/*
 * ============================================================================
 * FUNÇÃO: execute_command
 * ============================================================================
 * 
 * OBJETIVO:
 * Executa um único comando criando um processo filho.
 * 
 * PARÂMETROS:
 *   - cmd: o comando a executar (ex: "ls -la")
 * 
 * RETORNO:
 *   - PID do processo filho criado (se sucesso)
 *   - -1 se houve erro ou comando vazio
 * 
 * COMO FUNCIONA:
 * 1. Remove espaços no início do comando
 * 2. Faz o parsing do comando (separa programa e argumentos)
 * 3. Cria um processo filho com fork()
 * 4. O filho executa o comando com execvp()
 * 5. O pai retorna o PID do filho
 * 
 * EXEMPLO:
 *   cmd = "ls -la /tmp"
 *   Parsing resulta em:
 *     args[0] = "ls"
 *     args[1] = "-la"
 *     args[2] = "/tmp"
 *     args[3] = NULL
 */
pid_t execute_command(char *cmd) {
    
    // Remove espaços no início do comando
    while (*cmd == ' ') cmd++;
    
    // Se o comando está vazio, ignora
    if (strlen(cmd) == 0) {
        return -1;
    }

    /*
     * Faz uma cópia do comando porque strtok() modifica a string original
     */
    char cmd_copy[512];
    strncpy(cmd_copy, cmd, sizeof(cmd_copy) - 1);
    cmd_copy[sizeof(cmd_copy) - 1] = '\0';

    /*
     * ========================================================================
     * PARSING NÍVEL 2: Separar o comando em programa + argumentos
     * ========================================================================
     * 
     * Exemplo: "ls -la /tmp" é separado em:
     *   args[0] = "ls"      (o programa)
     *   args[1] = "-la"     (primeiro argumento)
     *   args[2] = "/tmp"    (segundo argumento)
     *   args[3] = NULL      (marca o fim do array)
     */
    char *args[32];  // Array para guardar os argumentos
    int i = 0;
    
    char *token = strtok(cmd_copy, " ");  // Primeiro token
    while (token != NULL && i < 31) {
        args[i++] = token;
        token = strtok(NULL, " ");  // Próximo token
    }
    args[i] = NULL;  // execvp() precisa que o array termine com NULL

    // Se não há argumentos, o comando é inválido
    if (args[0] == NULL) {
        return -1;
    }

    /*
     * ========================================================================
     * Criar processo filho com fork()
     * ========================================================================
     * 
     * fork() cria uma cópia do processo atual:
     * - Retorna 0 no processo FILHO
     * - Retorna o PID do filho no processo PAI
     * - Retorna -1 se houver erro
     */
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        /*
         * ====================================================================
         * PROCESSO FILHO - Executa o comando
         * ====================================================================
         * 
         * execvp() substitui o processo atual pelo programa especificado:
         * - args[0]: nome do programa a executar
         * - args: array com todos os argumentos
         * 
         * Se execvp() funcionar, o código abaixo NUNCA é executado
         * porque o processo filho foi substituído pelo novo programa.
         * 
         * Se chegarmos ao perror(), significa que execvp() falhou
         * (ex: comando não existe)
         */
        printf("[Servidor:Filho] A executar '%s'...\n", cmd);
        execvp(args[0], args);
        
        // Só chega aqui se execvp() falhar
        perror("[Servidor:Filho] Erro no execvp");
        _exit(EXIT_FAILURE);  // Usa _exit() em vez de exit() no filho
    }

    /*
     * ========================================================================
     * PROCESSO PAI - Retorna o PID do filho
     * ========================================================================
     * O pai não espera aqui pelo filho. Apenas retorna o PID para que
     * o main() possa esperar depois (permite execução de vários comandos).
     */
    return pid;
}


/*
 * ============================================================================
 * FUNÇÃO PRINCIPAL (main)
 * ============================================================================
 * 
 * Esta é a função que controla todo o servidor.
 * Cria o FIFO, fica à espera de mensagens, e processa-as.
 */
int main(void) {
    int fd;                   // Descritor do FIFO
    char buffer[MAX_BUFFER];  // Buffer para ler as mensagens

    /*
     * ========================================================================
     * PASSO 1: Criar a pasta de logs
     * ========================================================================
     * mkdir() cria a pasta se não existir.
     * Se já existir, não faz nada (ignora o erro).
     */
    mkdir("logs", 0777);

    /*
     * ========================================================================
     * PASSO 2: Criar o FIFO (named pipe)
     * ========================================================================
     * 
     * mkfifo() cria um ficheiro especial do tipo FIFO.
     * - FIFO_PATH: caminho do ficheiro
     * - 0666: permissões (rw-rw-rw-)
     * 
     * Se o FIFO já existir, mkfifo() retorna -1 e errno = EEXIST.
     * Nesse caso, ignoramos o erro e usamos o FIFO existente.
     */
    if (mkfifo(FIFO_PATH, 0666) == -1) {
        if (errno != EEXIST) {
            perror("mkfifo");
            exit(EXIT_FAILURE);
        }
        // Se errno == EEXIST, o FIFO já existe e podemos continuar
    }

    printf("[Servidor] A aguardar comandos no FIFO %s ...\n", FIFO_PATH);

    /*
     * ========================================================================
     * PASSO 3: Abrir o FIFO para leitura
     * ========================================================================
     * 
     * open() com O_RDONLY abre o FIFO apenas para leitura.
     * 
     * NOTA IMPORTANTE:
     * Esta chamada BLOQUEIA até que um cliente abra o FIFO para escrita!
     */
    fd = open(FIFO_PATH, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    /*
     * ========================================================================
     * PASSO 4: Loop principal - Processar mensagens
     * ========================================================================
     * 
     * O servidor fica num ciclo infinito:
     * 1. Lê uma mensagem do FIFO
     * 2. Processa os comandos
     * 3. Espera por mais mensagens
     */
    while (1) {
        /*
         * Lê dados do FIFO
         * - read() bloqueia até haver dados para ler
         * - Retorna o número de bytes lidos
         * - Retorna 0 quando o cliente fecha o FIFO (EOF)
         * - Retorna -1 se houver erro
         */
        ssize_t bytes = read(fd, buffer, sizeof(buffer) - 1);
        
        if (bytes > 0) {
            // Recebemos dados!
            buffer[bytes] = '\0';  // Adiciona terminador de string

            printf("[Servidor] Mensagem recebida: '%s'\n", buffer);

            /*
             * ================================================================
             * Arrays para guardar informação dos comandos
             * ================================================================
             * - pids[]: guarda o PID de cada processo filho
             * - commands[]: guarda uma cópia de cada comando (para o log)
             * - num_commands: conta quantos comandos foram lançados
             */
            pid_t pids[32];
            char *commands[32];
            int num_commands = 0;

            /*
             * ================================================================
             * PARSING NÍVEL 1: Separar os comandos por ';'
             * ================================================================
             * 
             * Exemplo: "ls -la;pwd;date" é separado em:
             *   - "ls -la"
             *   - "pwd"
             *   - "date"
             * 
             * Usamos strtok_r() em vez de strtok() porque é mais seguro
             * (strtok_r usa saveptr para guardar o estado)
             */
            char *saveptr1;
            char *cmd = strtok_r(buffer, ";", &saveptr1);
            
            while (cmd != NULL && num_commands < 32) {
                // Remove espaços no início do comando
                while (*cmd == ' ') cmd++;
                
                // Se o comando não está vazio
                if (strlen(cmd) > 0) {
                    /*
                     * strdup() faz uma cópia do comando.
                     * Precisamos disto porque:
                     * 1. strtok_r() modifica o buffer original
                     * 2. Queremos guardar o comando para escrever no log depois
                     */
                    commands[num_commands] = strdup(cmd);
                    
                    // Executa o comando (cria processo filho)
                    pids[num_commands] = execute_command(cmd);
                    
                    if (pids[num_commands] > 0) {
                        // Comando lançado com sucesso
                        num_commands++;
                    } else {
                        // Falhou - liberta a memória
                        free(commands[num_commands]);
                    }
                }
                
                // Próximo comando
                cmd = strtok_r(NULL, ";", &saveptr1);
            }

            printf("[Servidor] A executar %d comando(s)...\n", num_commands);

            /*
             * ================================================================
             * Esperar por todos os processos filhos
             * ================================================================
             * 
             * waitpid() espera que um processo filho termine:
             * - pids[i]: PID do filho a esperar
             * - &status: variável onde guarda o estado de saída
             * - 0: sem opções especiais
             * 
             * WIFEXITED(status): verifica se o filho terminou normalmente
             * WEXITSTATUS(status): obtém o código de saída (0 = sucesso)
             */
            for (int i = 0; i < num_commands; i++) {
                int status;
                waitpid(pids[i], &status, 0);

                // Prepara a entrada para o log
                char log_entry[512];
                
                if (WIFEXITED(status)) {
                    // O filho terminou normalmente
                    int exit_code = WEXITSTATUS(status);
                    snprintf(log_entry, sizeof(log_entry), "%s; exit status: %d\n", 
                             commands[i], exit_code);
                } else {
                    // O filho terminou de forma anormal (ex: signal)
                    snprintf(log_entry, sizeof(log_entry), "%s; terminou de forma anormal\n", 
                             commands[i]);
                }

                // Mostra e guarda o resultado
                printf("[Servidor] %s", log_entry);
                append_log(log_entry);
                
                // Liberta a memória da cópia do comando
                free(commands[i]);
            }

            if (num_commands > 0) {
                printf("[Servidor] Todos os %d comando(s) terminaram.\n", num_commands);
            }

        } else if (bytes == 0) {
            /*
             * ================================================================
             * Cliente fechou o FIFO (EOF)
             * ================================================================
             * Quando o cliente fecha a sua ponta do FIFO, read() retorna 0.
             * Temos de fechar e reabrir o FIFO para aceitar novos clientes.
             */
            printf("[Servidor] Cliente terminou a escrita. A reabrir FIFO...\n");
            close(fd);
            fd = open(FIFO_PATH, O_RDONLY);
            
        } else {
            // Erro na leitura
            perror("read");
            break;
        }
    }

    /*
     * ========================================================================
     * Limpeza final (nunca chega aqui no uso normal)
     * ========================================================================
     */
    close(fd);
    unlink(FIFO_PATH);  // Remove o ficheiro FIFO
    return 0;
}
