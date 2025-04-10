#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Definição de constantes para controlar o tamanho das entradas e saídas
#define MAXVARS 10       // Número máximo de variáveis proposicionais suportadas
#define MAXLEN 200       // Comprimento máximo da fórmula de entrada
#define TERM_LEN 50      // Tamanho máximo para cada termo ou cláusula gerada
#define RESULT_LEN 1000  // Tamanho máximo para a cadeia resultante (DNF ou CNF)

// Vetor global para armazenar as variáveis encontradas e contador delas

char vars[MAXVARS];
int numVars = 0;


// ---------------------------------------------------------------------------
// Função para registrar uma variável: verifica se 'c' já está em "vars"
// Se não estiver, adiciona e retorna o índice dela
// ---------------------------------------------------------------------------

int getVarIndex(char c) 
{
    for (int i = 0; i < numVars; i++) 
    {
        if (vars[i] == c) return i;
    }
    
    vars[numVars] = c;
    numVars++;
    return numVars - 1;
}

// ---------------------------------------------------------------------------
// Função para ignorar espaços em branco na expressão
// Avança o ponteiro "expr" enquanto o caractere atual for espaço
// ---------------------------------------------------------------------------

void skipSpaces(const char **expr) 
{
    while (**expr == ' ')
    {
        (*expr)++;
    }
}


// ---------------------------------------------------------------------------
// Declaração das funções do parser (analisador sintático) – funções recursivas
// que lidam com os diferentes níveis de precedência dos operadores.
// ---------------------------------------------------------------------------

int parseExpr(const char **expr, int *values);
int parseImp(const char **expr, int *values);
int parseOr(const char **expr, int *values);
int parseAnd(const char **expr, int *values);
int parseUnary(const char **expr, int *values);

// ---------------------------------------------------------------------------
// Funções auxiliares para os operadores de implicação e bicondicional
// ---------------------------------------------------------------------------

int implicacao(int a, int b) 
{
    // "a -> b" é equivalente a !a ou b
    return (!a) || b;
}


int bicondicional(int a, int b) 
{
    // "a <-> b" é equivalente a (a && b) ou (!a && !b)
    return (a && b) || ((!a) && (!b));
}


// ---------------------------------------------------------------------------
// Função parseUnary: trata de variáveis, negação (!) e expressões entre parênteses
// ---------------------------------------------------------------------------

int parseUnary(const char **expr, int *values) 
{
    skipSpaces(expr);
    int resultado = 0;
    
    // Se encontrar o operador de negação '!'
    if (**expr == '!') 
    {
        (*expr)++;  // Consome o '!'
        resultado = !parseUnary(expr, values);
        return resultado;
    }
    
    // Se encontrar um parêntese de abertura, processa a sub-expressão
    if (**expr == '(') 
    {
        (*expr)++;  // Consome o '('
        resultado = parseExpr(expr, values);
        skipSpaces(expr);
        
        if (**expr == ')') 
        {
            (*expr)++;  // Consome o ')'
        }
        
        return resultado;
    }
    
    // Se for uma variável (letra alfabética)
    if (isalpha(**expr)) 
    {
        int idx = getVarIndex(**expr);
        resultado = values[idx];
        (*expr)++;  // Avança após a variável
        return resultado;
    }
    
    return resultado;
}


// ---------------------------------------------------------------------------
// Função parseAnd: avalia expressões com o operador de conjunção '&'
// ---------------------------------------------------------------------------

int parseAnd(const char **expr, int *values) 
{
    int esquerdo = parseUnary(expr, values);
    skipSpaces(expr);
    
    while (**expr == '&') 
    {
        (*expr)++;  // Consome o '&'
        int direito = parseUnary(expr, values);
        esquerdo = esquerdo && direito;
        skipSpaces(expr);
    }
    
    return esquerdo;
}


// ---------------------------------------------------------------------------
// Função parseOr: avalia expressões com o operador de disjunção '|'
// ---------------------------------------------------------------------------

int parseOr(const char **expr, int *values) 
{
    int esquerdo = parseAnd(expr, values);
    skipSpaces(expr);
    
    while (**expr == '|') 
    {
        (*expr)++;  // Consome o '|'
        int direito = parseAnd(expr, values);
        esquerdo = esquerdo || direito;
        skipSpaces(expr);
    }
    
    return esquerdo;
}


// ---------------------------------------------------------------------------
// Função parseImp: avalia expressões com o operador de implicação "->"
// A implicação é associada à direita: A -> B -> C equivale a A -> (B -> C)
// ---------------------------------------------------------------------------

int parseImp(const char **expr, int *values) 
{
    int esquerdo = parseOr(expr, values);
    skipSpaces(expr);
    
    // Verifica a presença do operador "->"
    while (**expr == '-' && *((*expr)+1) == '>') 
    {
        (*expr) += 2;  // Consome "->"
        int direito = parseImp(expr, values);
        esquerdo = implicacao(esquerdo, direito);
        skipSpaces(expr);
    }
    
    return esquerdo;
}

// ---------------------------------------------------------------------------
// Função parseExpr: avalia expressões com o operador bicondicional "<->"
// O bicondicional também é associado à direita
// ---------------------------------------------------------------------------

int parseExpr(const char **expr, int *values) 
{
    int esquerdo = parseImp(expr, values);
    skipSpaces(expr);
    
    while (**expr == '<' && *((*expr)+1) == '-' && *((*expr)+2) == '>') 
    {
        (*expr) += 3;  // Consome "<->"
        int direito = parseExpr(expr, values);
        esquerdo = bicondicional(esquerdo, direito);
        skipSpaces(expr);
    }
    
    return esquerdo;
}

// ---------------------------------------------------------------------------
// Função que avalia uma fórmula a partir da string de expressão
// ---------------------------------------------------------------------------

int evalFormula(const char *expr, int *values) 
{
    const char *p = expr;
    return parseExpr(&p, values);
}

// ---------------------------------------------------------------------------
// Funções para construir as formas normais: DNF e CNF
//
// Para a DNF: Para cada atribuição que torna a fórmula verdadeira,
// constrói-se um termo (conjunção de literais), onde, para cada variável,
// se o valor é verdadeiro, usa a variável; se for falso, usa a negação.
//
// Para a CNF: Para cada atribuição que torna a fórmula falsa,
// constrói-se uma cláusula (disjunção de literais), onde, para cada variável,
// se o valor é falso, usa a variável; se for verdadeiro, usa a negação.
// ---------------------------------------------------------------------------

void buildDNFTerm(int *values, char *term)
{
    strcpy(term, "(");
    int primeiro = 1;
    
    for (int i = 0; i < numVars; i++) 
    {
        if (!primeiro) 
        {
            strcat(term, " & ");
        }
        primeiro = 0;
        char literal[TERM_LEN];
        if (values[i])
            sprintf(literal, "%c", vars[i]);
        else
            sprintf(literal, "!%c", vars[i]);
        strcat(term, literal);
    }
    
    strcat(term, ")");
}


void buildCNFClause(int *values, char *clause) 
{
    strcpy(clause, "(");
    int primeiro = 1;
    
    for (int i = 0; i < numVars; i++) 
    {
        if (!primeiro) 
        {
            strcat(clause, " | ");
        }
        primeiro = 0;
        char literal[TERM_LEN];
        if (!values[i])
            sprintf(literal, "%c", vars[i]);
        else
            sprintf(literal, "!%c", vars[i]);
        strcat(clause, literal);
    }
    
    strcat(clause, ")");
}


int main() 
{
    char formula[MAXLEN];
    char dnf[RESULT_LEN] = "";
    char cnf[RESULT_LEN] = "";
    
    // Solicita ao usuário que insira uma fórmula da lógica proposicional
  
    printf("Digite uma fórmula proposicional (ex: (P|Q) -> R)(| = OR , & = AND e ! = NAO) :\n");
    scanf(" %[^\n]", formula);
    
    // Reinicia o contador de variáveis e registra todas as variáveis presentes na fórmula
  
    numVars = 0;
    for (int i = 0; formula[i] != '\0'; i++) 
    {
        if (isalpha(formula[i])) 
        {
            getVarIndex(formula[i]);
        }
    }
    
    // Calcula o número total de atribuições possíveis (2^(número de variáveis))
  
    int total = 1 << numVars;
    
    // Buffers para acumular os termos da DNF e as cláusulas da CNF
  
    char dnfTerms[RESULT_LEN] = "";
    char cnfClauses[RESULT_LEN] = "";
    int dnfCount = 0, cnfCount = 0;
    
    // Percorre todas as linhas da tabela verdade
  
    for (int i = 0; i < total; i++) 
    {
        int values[MAXVARS] = {0};
      
        // Define o valor de cada variável para essa atribuição
      
        for (int j = 0; j < numVars; j++) 
        {
            values[j] = (i >> j) & 1;
        }
      
        // Avalia a fórmula para a atribuição atual
      
        int resultado = evalFormula(formula, values);
        char term[TERM_LEN];
        if (resultado) 
        {
            // Se a fórmula for verdadeira, constrói um termo para a DNF
            buildDNFTerm(values, term);
            if (dnfCount > 0) 
            {
                strcat(dnfTerms, " | ");
            }
            strcat(dnfTerms, term);
            dnfCount++;
        } 
        else 
        {
            // Se a fórmula for falsa, constrói uma cláusula para a CNF
            buildCNFClause(values, term);
            if (cnfCount > 0) 
            {
                strcat(cnfClauses, " & ");
            }
            strcat(cnfClauses, term);
            cnfCount++;
        }
    }
    
    // Tratamento de casos especiais:
    // Se nenhum termo foi gerado na DNF, a fórmula é insatisfatível
  
    if (dnfCount == 0) 
    {
        strcpy(dnf, "False");
    }
    else 
    {
        strcpy(dnf, dnfTerms);
    }
    
    // Se nenhuma cláusula foi gerada na CNF, a fórmula é uma tautologia
  
    if (cnfCount == 0) 
    {
        strcpy(cnf, "True");
    }
    else 
    {
        strcpy(cnf, cnfClauses);
    }
    
    // Exibe as formas normais geradas
  
    printf("\nForma Normal Disjuntiva (DNF): %s\n", dnf);
    printf("Forma Normal Conjuntiva (CNF): %s\n\n", cnf);
    
    printf("| = OR , & = AND e ! = NAO");
    
    return 0;
}
