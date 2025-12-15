# NS-3 Simulator: Guia de Instalação e Configuração

## 1. Visão Geral
O **ns-3** é um simulador de redes de eventos discretos, focado em pesquisa e uso educacional. Este ambiente está configurado para suportar pesquisas em:
* **Protocolos de Internet:** IPv4/IPv6, TCP/UDP, Roteamento.
* **Wireless e IoT:** Wi-Fi (até 802.11ax), LTE/5G, LoRaWAN, IEEE 802.15.4 (ZigBee).
* **Integração com IA:** Interface via memória compartilhada para frameworks de ML.

**Stack Tecnológico:**
* **Core:** C++ (Performance e Gerenciamento de Memória).
* **Bindings:** Python (Scripting rápido e prototipagem).
* **Build System:** CMake + Ninja (versões ns-3.36+).
---

## 2. Configurações da Estação Rádio Base (gNodeB)
A replicação do modelo de consumo de potência da estação rádio base (BTS), conforme proposto no artigo de referência, foi realizada através de uma arquitetura de simulação distribuída que interliga o Simulador ns-3/5G-LENA.

A tabela a seguir resume os parâmetros críticos de rádio (RF) e de rede (EPC) utilizados na configuração do gNodeB, conforme o cenário 5G FWA implementado em `fwa-network-sim.cc`.

| Parâmetro | Valor Configurado | Notas |
| :--- | :--- | :--- |
| **Padrão** | NR (5G) | Implementação via 5G-LENA. |
| **Frequência Central** | 3.5 GHz ($3.5 \times 10^9$ Hz) | Banda n77. |
| **Largura de Banda (BW)** | 40 MHz ($40 \times 10^6$ Hz) | Define a capacidade máxima de dados. |
| **Numerologia ($\mu$)** | 1 | Corresponde a um *Subcarrier Spacing* (SCS) de 30 kHz. |
| **Capacidade Máxima** | 270 RBs | Número máximo de Blocos de Recurso (RBs) disponíveis. |
| **Modelo de Canal** | ThreeGppChannelModel | Modelo de propagação do 3GPP para ambientes de exterior. |
| **Função (EPC)** | Conectado ao Core P2P | O gNodeB atua como porta de acesso (RAN) ao *Core Network*. |

## 3. Pré-requisitos do Sistema (Linux/Ubuntu)

Antes de iniciar, é necessário instalar as ferramentas de compilação, bibliotecas de desenvolvimento e dependências do Python.

```bash
# Atualizar repositórios
sudo apt update && sudo apt upgrade -y

# Ferramentas de Build (C++ e CMake)
sudo apt install -y build-essential cmake ninja-build g++ gcc

# Dependências Python (para bindings e visualização)
sudo apt install -y python3 python3-dev python3-pip python3-setuptools gir1.2-goocanvas-2.0 python3-gi python3-gi-cairo python3-pygraphviz

# Ferramentas de Debug e Análise
sudo apt install -y gdb valgrind tcpdump wireshark

# Bibliotecas XML e GSL (GNU Scientific Library - importante para modelos matemáticos de sinal)
sudo apt install -y libxml2 libxml2-dev libgsl-dev
```

## 4. Instalação do NS-3
Recomenda-se o uso do release oficial para estabilidade em trabalhos de mestrado.

### 4.1 Download
Vamos instalar a versão ns-3.40 (ou a mais recente estável).

```bash
cd ~
mkdir workspace
cd workspace

# Download do pacote all-in-one (inclui dependências opcionais como NetAnim)
wget [https://www.nsnam.org/release/ns-allinone-3.40.tar.bz2](https://www.nsnam.org/release/ns-allinone-3.40.tar.bz2)

# Descompactar
tar xjf ns-allinone-3.40.tar.bz2

# Entrar no diretório do simulador
cd ns-allinone-3.40/ns-3.40
```
### 4.2 Configuração e Compilação
O ns-3 utiliza um script wrapper (ns3) que gerencia o CMake.

Configurar o Build: Habilitamos os exemplos e testes para verificar a instalação. O modo debug é ideal para desenvolvimento (permite gdb), enquanto optimized é para coleta de dados finais.

```bash
./ns3 configure --enable-examples --enable-tests --build-profile=debug
```
Compilar (Build): Este passo pode demorar dependendo do processador (15-40 minutos).

## 5.Instalação e Configuração do Módulo 5G-LENA (NR)
O simulador depende do módulo externo 5G-LENA (nr) para fornecer a funcionalidade 5G. Siga os passos abaixo para garantir que o módulo esteja instalado e que o ambiente ns-3 o reconheça corretamente.

### 5.1 Clonagem do Módulo
O módulo deve ser clonado dentro da pasta contrib/ do seu repositório ns-3 para ser detectado pelo sistema de build do ns-3.
```bash
# Navegue até a pasta contrib/
cd /ns-3-dev/contrib/

# Clone o repositório 5G-LENA
git clone https://github.com/5G-LENA/nr.git
```
### 5.2 Instalação das Dependências (Pré-requisito)
O módulo nr requer cabeçalhos de desenvolvimento e bibliotecas externas. Este passo é crucial para evitar o erro Python.h: No such file or directory e garantir que o pybind11 funcione:
```bash
# 1. Instalar cabeçalhos de desenvolvimento do Python
sudo apt update
sudo apt install python3-dev

# 2. Instalar bibliotecas C++ necessárias (Eigen3 e GSL)
sudo apt install libeigen3-dev libgsl-dev
```
### 5.3 Configuração do Ambiente ns-3
Após a instalação das dependências, é necessário configurar o ns-3 para reconhecer o novo módulo e as bibliotecas:
```bash
# Retorne à pasta raiz do ns-3
cd /ns-3-dev

# Configurar o ns-3, habilitando os bindings Python e o pybind11
./ns3 configure --enable-examples --enable-tests --enable-python-bindings -- --with-pybind11
```
Verificação: Após a execução do comando configure, certifique-se de que o módulo nr e o python-bindings estejam listados como enabled ou OK na saída do console.

### 5.4 Compilação Completa
Finalmente, compile todo o projeto para integrar o novo módulo:
```bash
./ns3 build
```
## 6. Verificação da Instalação
Após a compilação, é crucial validar se os módulos principais (especialmente Wi-Fi, LTE e CSMA) estão funcionais.

```bash
# Rodar a suite de testes unitários
./ns3 test
```
Se o output final indicar "PASS", o núcleo do simulador está íntegro.

## 7. Executando sua Primeira Simulação ("Hello World")
O ns-3 vem com scripts de exemplo na pasta examples/. Vamos rodar um exemplo simples de UDP Echo (cliente-servidor).

Comando:

```bash
./ns3 run first
```
Saída Esperada:
```bash
Plaintext

At time 2s client sent 1024 bytes to 10.1.1.2 Port 9
At time 2.00369s server received 1024 bytes from 10.1.1.1 Port 49153
At time 2.00369s server sent 1024 bytes to 10.1.1.1 Port 49153
At time 2.00737s client received 1024 bytes from 10.1.1.2 Port 9
```
Interpretação: O cliente enviou um pacote aos 2 segundos. Devido à latência simulada do canal, o servidor recebeu 3.69ms depois.

## 8. Estrutura de Diretórios para Desenvolvimento
Para manter o projeto organizado, evite editar arquivos dentro de examples/. Crie seus scripts na pasta scratch/. O compilador detecta automaticamente arquivos .cc nesta pasta.

* src/: Código fonte dos módulos (Wi-Fi, LTE, Internet).

* build/: Binários e bibliotecas compiladas.

* scratch/: Seu laboratório. Coloque seus experimentos aqui.

Exemplo: scratch/meu-experimento-iot.cc

Executar:
```bash
./ns3 run meu-experimento-iot
```
