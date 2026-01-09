# 🚀 Sistema Cliente-Servidor de Execução Remota | Projeto SO 25/26

<div align="center">

**Sistema de execução remota de programas não-interativos usando Named Pipes (FIFO)**

[![C](https://img.shields.io/badge/C-00599C?style=flat&logo=c&logoColor=white)](https://www.iso.org/standard/74528.html)
[![Linux](https://img.shields.io/badge/Linux-FCC624?style=flat&logo=linux&logoColor=black)](https://www.kernel.org/)
[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

</div>

---

## 📋 Índice

- [Sobre o Projeto](#-sobre-o-projeto)
- [Características](#-características)
- [Arquitetura](#-arquitetura)
- [Requisitos](#-requisitos)
- [Instalação](#-instalação)
- [Utilização](#-utilização)
- [Estrutura do Projeto](#-estrutura-do-projeto)
- [Implementação Técnica](#-implementação-técnica)
- [Logs](#-logs)
- [Exemplos](#-exemplos)
- [Limitações Conhecidas](#-limitações-conhecidas)
- [Autor](#-autor)

---

## 🎯 Sobre o Projeto

Sistema cliente-servidor desenvolvido em **C puro** para execução remota de programas não-interativos, utilizando exclusivamente **syscalls** e **Named Pipes (FIFO)** para comunicação entre processos.

### Objetivos Pedagógicos

- ✅ Consolidar conhecimentos de **gestão de processos** (`fork`, `exec`, `wait`)
- ✅ Dominar **gestão de ficheiros** e **IPC** (Inter-Process Communication)
- ✅ Implementar **execução concorrente** de múltiplos comandos
- ✅ Aplicar **tratamento de sinais** e **prevenção de processos zombie**
- ✅ Desenvolver **logging profissional** com timestamps

---

## ✨ Características

### 🔥 Funcionalidades Core

- **Comunicação via Named Pipes (FIFO)** - IPC robusto e eficiente
- **Protocolo personalizado** - Separação de comandos por `;`
- **Execução concorrente** - Múltiplos comandos executam em paralelo
- **Logging automático** - Histórico completo com timestamps
- **Signal handling** - Encerramento gracioso e cleanup automático
- **Validação robusta** - Verificação de limites e tratamento de erros

### 🛡️ Segurança e Robustez

- ✅ **Zero funções de alto nível** - Apenas syscalls puras (`open`, `read`, `write`, etc.)
- ✅ **Tratamento de sinais** - SIGINT, SIGTERM, SIGCHLD
- ✅ **Prevenção de zombies** - Handler SIGCHLD automático
- ✅ **Validação de buffers** - Proteção contra overflow
- ✅ **Gestão de memória** - `free()` correto de recursos alocados

### 📊 Funcionalidades Avançadas

- **Timestamps precisos** - Formatação manual sem `strftime()`
- **Concorrência real** - `waitpid(-1, ...)` para paralelismo verdadeiro
- **Logs estruturados** - Formato `[TIMESTAMP] comando; exit status: N`
- **Múltiplos clientes** - Suporte a conexões sequenciais

---

## 🏗️ Arquitetura

```
┌─────────────┐                           ┌─────────────┐
│   Cliente   │                           │  Servidor   │
│             │                           │             │
│  Envia      │    Named Pipe (FIFO)     │  Recebe     │
│  comandos   │ ─────────────────────────▶│  comandos   │
│             │   /tmp/exec_fifo          │             │
└─────────────┘                           └──────┬──────┘
                                                  │
                                                  │ fork()
                                                  ▼
                                          ┌───────────────┐
                                          │ Proc. Filho 1 │
                                          │   execvp()    │
                                          └───────────────┘
                                          ┌───────────────┐
                                          │ Proc. Filho 2 │
                                          │   execvp()    │
                                          └───────────────┘
                                          ┌───────────────┐
                                          │ Proc. Filho N │
                                          │   execvp()    │
                                          └───────────────┘
                                                  │
                                                  │ waitpid(-1)
                                                  ▼
                                          ┌───────────────┐
                                          │  logs/*.log   │
                                          │  [Timestamp]  │
                                          └───────────────┘
```

### Fluxo de Execução

1. **Cliente** serializa comandos (`cmd1;cmd2;cmd3`)
2. **Servidor** recebe via FIFO
3. **Parsing** separa comandos por `;`
4. **Fork** cria processo filho para cada comando
5. **Exec** substitui filho pelo programa
6. **Wait** aguarda todos os filhos (concorrente)
7. **Log** regista resultados com timestamp

---

## 💻 Requisitos

### Sistema Operativo

- **Linux** (Kernel 2.6+)
- **macOS** (com Multipass/VM Ubuntu)

### Ferramentas

- `gcc` (GNU Compiler Collection)
- `make` (Build automation)
- `git` (opcional)

### Ferramentas

- `gcc` (GNU Compiler Collection)
- `make` (Build automation)
- `git` (opcional)

---

## 🚀 Instalação

### Opção 1: Linux Nativo

```bash
# Clonar repositório
git clone https://github.com/03lucasmaciel/me-so-pipes.git
cd me-so-pipes

# Compilar
make

# Executar (2 terminais)
# Terminal 1:
./build/server

# Terminal 2:
./build/client "ls -la" "pwd" "date"
```

### Opção 2: macOS (via Multipass)

### Opção 2: macOS (via Multipass)

```bash
# Criar VM Ubuntu
multipass launch 24.04 --name so-projeto --cpus 2 --mem 2G --disk 10G

# Aceder à VM
multipass shell so-projeto

# Instalar ferramentas
sudo apt update && sudo apt upgrade -y
sudo apt install -y build-essential make gcc git

# Clonar e compilar
git clone https://github.com/03lucasmaciel/me-so-pipes.git
cd me-so-pipes
make
```

---

## 📖 Utilização

### Iniciar o Servidor

```bash
./build/server
```

**Output esperado:**

```
[Servidor] A aguardar comandos no FIFO /tmp/exec_fifo ...
[Servidor] Pressiona Ctrl+C para terminar.
```

### Enviar Comandos (Cliente)

**Comando único:**

```bash
./build/client "ls -la"
```

**Múltiplos comandos (execução concorrente):**

```bash
./build/client "ls -la" "pwd" "date" "whoami"
```

**Comandos com argumentos:**

```bash
./build/client "echo Hello World" "uname -a" "df -h"
```

### Encerrar o Servidor

Pressionar `Ctrl+C` faz cleanup automático (remove FIFO).

---

## 📁 Estrutura do Projeto

```
me-so-pipes/
├── LICENSE                 # Licença MIT
├── Makefile               # Build automation
├── README.md              # Este ficheiro
├── build/                 # Executáveis compilados
│   ├── server            # Servidor
│   └── client            # Cliente
├── logs/                  # Ficheiros de log
│   └── server.log        # Histórico de execuções
└── src/                   # Código-fonte
    ├── server.c          # Implementação do servidor (824 linhas)
    └── client.c          # Implementação do cliente (270 linhas)
```

---

## 🔧 Implementação Técnica

### Syscalls Utilizadas

| Syscall       | Função            | Utilização                    |
| ------------- | ----------------- | ----------------------------- |
| `mkfifo()`    | Criar FIFO        | Criação do named pipe         |
| `open()`      | Abrir ficheiro    | Abertura do FIFO e logs       |
| `close()`     | Fechar ficheiro   | Libertar recursos             |
| `read()`      | Ler dados         | Receber comandos do FIFO      |
| `write()`     | Escrever dados    | Enviar comandos e logs        |
| `fork()`      | Criar processo    | Criar filho para comando      |
| `execvp()`    | Executar programa | Substituir filho pelo comando |
| `waitpid()`   | Esperar filho     | Sincronização de processos    |
| `signal()`    | Registar handler  | Tratamento de sinais          |
| `unlink()`    | Remover ficheiro  | Cleanup do FIFO               |
| `time()`      | Obter timestamp   | Logging temporal              |
| `localtime()` | Converter tempo   | Formatação de timestamps      |

### Características de Implementação

#### 1. **Execução Concorrente Real**

```c
// ✅ Implementação correta
for (int i = 0; i < num_commands; i++) {
    pid_t terminated_pid = waitpid(-1, &status, 0);  // Espera por QUALQUER filho
    // Mapeia PID → comando para logging correto
}
```

**Benefício:** Comandos executam verdadeiramente em paralelo. Se `cmd2` terminar antes de `cmd1`, é processado imediatamente.

#### 2. **Signal Handling Robusto**

```c
signal(SIGINT, signal_handler);    // Ctrl+C
signal(SIGTERM, signal_handler);   // kill <pid>
signal(SIGCHLD, sigchld_handler);  // Previne zombies
```

**Benefício:** Encerramento gracioso e prevenção de processos órfãos.

#### 3. **Logging Profissional**

```c
[2026-01-09 11:30:45] ls -la; exit status: 0
[2026-01-09 11:30:45] pwd; exit status: 0
[2026-01-09 11:30:46] date; exit status: 0
```

**Timestamps formatados manualmente** (sem `strftime()`) mantendo pureza das syscalls.

#### 4. **Funções Auxiliares Puras**

```c
void print_str(const char *str);           // Substitui printf()
void print_err(const char *str);           // Escreve no stderr
int print_int(int fd, int num);            // Converte e imprime int
void print_error(const char *msg);         // Substitui perror()
int format_timestamp(char *buf, int size); // Formata tempo manualmente
```

**Nota:** Zero uso de `printf()`, `perror()`, `snprintf()` - apenas `write()`.

---

## 📊 Logs

### Localização

```bash
logs/server.log
```

### Formato

```
[YYYY-MM-DD HH:MM:SS] comando argumentos; exit status: N
```

### Exemplos

**Execução bem-sucedida:**

```
[2026-01-09 11:30:45] ls -la; exit status: 0
[2026-01-09 11:30:45] pwd; exit status: 0
[2026-01-09 11:30:46] date; exit status: 0
```

**Comando inexistente:**

```
[2026-01-09 11:31:20] comandoinvalido; exit status: 127
```

**Terminação anormal:**

```
[2026-01-09 11:32:15] sleep 100; terminou de forma anormal
```

### Visualizar Logs

```bash
# Mostrar todo o histórico
cat logs/server.log

# Últimas 10 entradas
tail -10 logs/server.log

# Seguir em tempo real
tail -f logs/server.log

# Filtrar por exit status
grep "exit status: 0" logs/server.log
```

---

## 🧪 Exemplos

### Exemplo 1: Comandos Básicos

```bash
./build/client "ls" "pwd" "whoami"
```

**Output do servidor:**

```
[Servidor] Mensagem recebida: 'ls;pwd;whoami'
[Servidor] A executar 3 comando(s)...
[Servidor:Filho] A executar 'ls'...
[Servidor:Filho] A executar 'pwd'...
[Servidor:Filho] A executar 'whoami'...
build  logs  Makefile  README.md  src
/home/user/me-so-pipes
user
[Servidor] ls; exit status: 0
[Servidor] pwd; exit status: 0
[Servidor] whoami; exit status: 0
[Servidor] Todos os 3 comando(s) terminaram.
```

### Exemplo 2: Comandos com Argumentos

```bash
./build/client "echo 'Hello World'" "uname -a" "df -h"
```

### Exemplo 3: Teste de Concorrência

```bash
# Comandos com durações diferentes
./build/client "sleep 5 && echo LENTO" "echo RAPIDO" "sleep 2 && echo MEDIO"
```

**Output (ordem de terminação):**

```
RAPIDO
[Servidor] echo RAPIDO; exit status: 0      ← Termina 1º
MEDIO
[Servidor] sleep 2 && echo MEDIO; exit status: 0   ← Termina 2º
LENTO
[Servidor] sleep 5 && echo LENTO; exit status: 0   ← Termina 3º
```

### Exemplo 4: Verificar Processos
```bash
# Verificar processos zombie
ps aux | grep defunct  # Não deve mostrar nenhum <defunct>
```

---

## ⚠️ Limitações Conhecidas

### 1. Parser Simples

**Limitação:** O parser separa argumentos apenas por espaços e **não respeita aspas**.

**Exemplo:**

```bash
./build/client "echo 'hello world'"
```

**Comportamento:**

- Parseado como: `["echo", "'hello", "world'"]`
- Output: `'hello world'` (com as aspas)

**Justificação:** Simplificação aceável para âmbito académico. Parser com suporte a aspas exigiria máquina de estados complexa.

**Workaround:**

```bash
# Em vez de:
./build/client "echo 'hello world'"

# Usar:
./build/client "echo hello_world"
```

### 2. Limite de Comandos

- **Máximo 32 comandos** por mensagem
- **Máximo 4096 bytes** por mensagem total
- **Máximo 511 caracteres** por comando individual

### 3. Compatibilidade

- **Requer Linux** (ou VM com Linux no macOS)
- **Named Pipes** não funcionam em Windows nativamente

---

## 🎓 Decisões de Design

### Por que Syscalls Puras?

**Decisão:** Usar apenas `write()` em vez de `printf()`.

**Razão:** Enunciado exige "evitar funções de alto nível". Isto demonstra:

- Compreensão profunda de I/O de baixo nível
- Controlo total sobre operações do sistema
- Conformidade estrita com requisitos académicos

### Por que `waitpid(-1, ...)`?

**Decisão:** Esperar por QUALQUER filho em vez de ordem específica.

**Razão:** Permite execução **verdadeiramente concorrente**:

```c
// ❌ Errado (sequencial)
for (i = 0; i < n; i++) {
    waitpid(pids[i], ...);  // Bloqueia até pids[i] terminar
}

// ✅ Correto (concorrente)
for (i = 0; i < n; i++) {
    waitpid(-1, ...);  // Processa primeiro que terminar
}
```

### Por que SIGCHLD Handler?

**Decisão:** Adicionar handler mesmo já fazendo `waitpid()` no main.

**Razão:** Safety net duplo:

- **Layer 1:** Main loop faz wait normalmente
- **Layer 2:** Handler previne zombies se houver crash entre fork/wait

---

## 🛠️ Compilação

### Makefile

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -O2

all: build/server build/client

build/server: src/server.c
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $^

build/client: src/client.c
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf build/* logs/* /tmp/exec_fifo
```

### Comandos

```bash
# Compilar tudo
make

# Compilar só servidor
make build/server

# Compilar só cliente
make build/client

# Limpar (remove executáveis, logs e FIFO)
make clean

# Recompilar tudo
make clean && make
```

---

## 🐛 Troubleshooting

### Problema: "No such file or directory" ao executar cliente

**Causa:** Servidor não está a correr (FIFO não foi criado).

**Solução:**

```bash
# Terminal 1 - Iniciar servidor PRIMEIRO
./build/server

# Terminal 2 - Depois executar cliente
./build/client "ls"
```

### Problema: Cliente bloqueia indefinidamente

**Causa:** FIFO órfão (servidor crashou sem fazer cleanup).

**Solução:**

```bash
# Remover FIFO manualmente
rm /tmp/exec_fifo

# Reiniciar servidor
./build/server
```

### Problema: "Address already in use" ou FIFO já existe

**Solução:**

```bash
# Limpar FIFO e recompilar
make clean
make
./build/server
```

### Problema: Processos zombie

**Verificar:**

```bash
ps aux | grep defunct
```

**Nota:** Com SIGCHLD handler implementado, isto **não deve acontecer**.

---

## 📈 Melhorias Futuras (Fora do Âmbito)

- [ ] **Parser avançado** com suporte a aspas e escapes
- [ ] **Redirecionamento** de I/O (`>`, `<`, `|`)
- [ ] **Comunicação bidirecional** (servidor responde ao cliente)
- [ ] **Autenticação** de clientes
- [ ] **Limite de timeout** para comandos
- [ ] **Múltiplos clientes simultâneos** (threads/select)
- [ ] **Compressão** de logs antigos
- [ ] **Interface web** para monitorização



---

## 📚 Referências

- [Linux Manual Pages](https://man7.org/linux/man-pages/)
- [Advanced Programming in the UNIX Environment](https://www.apuebook.com/)
- [The Linux Programming Interface](https://man7.org/tlpi/)
- [POSIX.1-2017](https://pubs.opengroup.org/onlinepubs/9699919799/)

---