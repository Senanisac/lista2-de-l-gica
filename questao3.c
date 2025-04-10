#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> // Para usar as funcoes isalpha e isspace

#define MAXVARS 10
#define MAXLEN 200 // Maxi para digitar

char vars[MAXVARS];   // Armazena as variáveis encontradas
int numVars = 0;      // Número de variáveis


// Função para registrar e retornar o índice de uma variável
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

// Avança o ponteiro ignorando espaços
void skipSpaces(const char **expr) 
{
    while (**expr == ' ') 
    {
        (*expr)++;
    }
}

// Função que converte valores booleanos para o operador de implicação
// Implicação: A -> B equivale a (!A || B)
int implicacao(int a, int b) 
{
    return (!a) || b;
}

// Função que converte valores booleanos para o operador bicondicional
// Bicondicional: A <-> B equivale a ((A && B) || (!A && !B))
int bicondicional(int a, int b) 
{
    return (a && b) || ((!a) && (!b));
}

// Forward declarations para as funções do parser
int parseExpr(const char **expr, int *values);
int parseImp(const char **expr, int *values);
int parseOr(const char **expr, int *values);
int parseAnd(const char **expr, int *values);
int parseUnary(const char **expr, int *values);


// parseUnary: trata variáveis, negação e subexpressões entre parênteses
int parseUnary(const char **expr, int *values) 
{
    skipSpaces(expr);
    int result = 0;
    
    if (**expr == '!') 
    { // Operador de negação
        (*expr)++;
        result = !parseUnary(expr, values);
        return result;
    }
    
    if (**expr == '(')
    {  // Parênteses: avalia o que está dentro
        (*expr)++; // descarta '('
        result = parseExpr(expr, values);
        skipSpaces(expr);
        if (**expr == ')') 
        {
            (*expr)++; // descarta ')'
        }
        return result;
    }
    
    // Se for letra, trata como variável
    if (isalpha(**expr)) 
    {
        int idx = getVarIndex(**expr);
        result = values[idx];
        (*expr)++;
        return result;
    }
    
    return result;
}



// parseAnd: avalia expressões com conjunção (&)
int parseAnd(const char **expr, int *values) 
{
    int left = parseUnary(expr, values);
    skipSpaces(expr);
    
    while (**expr == '&') 
    {
        (*expr)++;  // consome '&'
        int right = parseUnary(expr, values);
        left = left && right;
        skipSpaces(expr);
    }
    return left;
}



// parseOr: avalia expressões com disjunção (|)
int parseOr(const char **expr, int *values) 
{
    int left = parseAnd(expr, values);
    skipSpaces(expr);
    
    while (**expr == '|')
    {
        (*expr)++;  // consome '|'
        int right = parseAnd(expr, values);
        left = left || right;
        skipSpaces(expr);
    }
    return left;
}



// parseImp: avalia implicação (->)
// A implicação é associada à direita, isto é, A -> B -> C equivale a A -> (B -> C)
int parseImp(const char **expr, int *values) 
{
    int left = parseOr(expr, values);
    skipSpaces(expr);
    
    // Verifica se há o operador "->"
    while (**expr == '-' && *((*expr) + 1) == '>') 
    {
        (*expr) += 2;  // consome "->"
        int right = parseImp(expr, values);
        left = implicacao(left, right);
        skipSpaces(expr);
    }
    
    return left;
}



// parseExpr: avalia o operador bicondicional (<->)
// O operador bicondicional também é associado à direita
int parseExpr(const char **expr, int *values)
{
    int left = parseImp(expr, values);
    skipSpaces(expr);  //Ignorar as espacos na expressao
    
    // Verifica se há o operador "<->"
    while (**expr == '<' && *((*expr)+1) == '-' && *((*expr)+2) == '>') 
    {
        (*expr) += 3;  // consome "<->"
        int right = parseExpr(expr, values);
        left = bicondicional(left, right);
        skipSpaces(expr);
    }
    
    return left;
}



// Avalia uma fórmula a partir de sua expressão
int evalFormula(const char *expr, int *values) 
{
    const char *p = expr;
    return parseExpr(&p, values);
}



// Verifica se duas fórmulas são logicamente equivalentes
int isEquivalent(const char *f1, const char *f2) 
{
    int total = 1 << numVars; // Número de linhas na tabela verdade (2^(número de variáveis))
    
    for (int i = 0; i < total; i++) 
    {
        int values[MAXVARS] = {0};
        
        for (int j = 0; j < numVars; j++) 
        {
            values[j] = (i >> j) & 1;
        }
        
        int v1 = evalFormula(f1, values);
        int v2 = evalFormula(f2, values);
        if (v1 != v2) return 0;
    }
    
    return 1;
}



int main() 
{
    char f1[MAXLEN], f2[MAXLEN];
    
    printf("| = OR , & = AND e ! = NAO\n\n");
    
    printf("Digite a primeira fórmula (ex: (P|Q)) (| = OR , & = AND e ! = NAO) :\n");
    scanf(" %[^\n]", f1);
    printf("Digite a segunda fórmula (ex: (!( !P & !Q ))) ou outra equivalente :\n");
    scanf(" %[^\n]", f2);

    // Reinicia as variáveis para detectar as presentes nas fórmulas
    numVars = 0;
    // Identifica variáveis na primeira fórmula
    for (int i = 0; f1[i] != '\0'; i++) 
    {
        if (isalpha(f1[i])) 
        {
            getVarIndex(f1[i]);
        }
    }
    // Identifica variáveis na segunda fórmula
    for (int i = 0; f2[i] != '\0'; i++) 
    {
        if (isalpha(f2[i])) 
        {
            getVarIndex(f2[i]);
        }
    }

    if (isEquivalent(f1, f2))
        printf("As fórmulas SÃO logicamente equivalentes.\n");
    else
        printf("As fórmulas NÃO são logicamente equivalentes.\n");
    
    return 0;
}
