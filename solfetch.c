#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_TOKENS 1000
#define MAX_RESPONSE 65536
#define DEFAULT_LIMIT 10

typedef struct {
    char mint[45];
    char symbol[32];
    double amount;
    int decimals;
} Token;

char* curl_request(const char* url, const char* data) {
    char command[2048];
    snprintf(command, sizeof(command),
        "curl -s -X POST -H 'Content-Type: application/json' -d '%s' '%s'",
        data, url);
    
    FILE* fp = popen(command, "r");
    if (!fp) return NULL;
    
    char* response = malloc(MAX_RESPONSE);
    if (!response) {
        pclose(fp);
        return NULL;
    }
    
    size_t total = 0;
    size_t bytes;
    while ((bytes = fread(response + total, 1, MAX_RESPONSE - total - 1, fp)) > 0) {
        total += bytes;
        if (total >= MAX_RESPONSE - 1) break;
    }
    
    response[total] = '\0';
    pclose(fp);
    
    return response;
}

char* extract_json_string(const char* json, const char* key) {
    char search_key[128];
    snprintf(search_key, sizeof(search_key), "\"%s\":", key);
    
    char* pos = strstr(json, search_key);
    if (!pos) return NULL;
    
    pos += strlen(search_key);
    while (*pos == ' ' || *pos == '\t') pos++;
    
    if (*pos != '"') return NULL;
    pos++;
    
    char* end = strchr(pos, '"');
    if (!end) return NULL;
    
    int len = end - pos;
    char* result = malloc(len + 1);
    if (!result) return NULL;
    
    strncpy(result, pos, len);
    result[len] = '\0';
    
    return result;
}

double extract_json_number(const char* json, const char* key) {
    char search_key[128];
    snprintf(search_key, sizeof(search_key), "\"%s\":", key);
    
    char* pos = strstr(json, search_key);
    if (!pos) return 0.0;
    
    pos += strlen(search_key);
    while (*pos == ' ' || *pos == '\t') pos++;
    
    return atof(pos);
}

void parse_token_accounts(const char* json, Token* tokens, int* count) {
    *count = 0;
    
    char* accounts_start = strstr(json, "\"value\":[");
    if (!accounts_start) return;
    
    char* pos = accounts_start + 9;
    
    while (*pos && *count < MAX_TOKENS) {
        char* account_start = strchr(pos, '{');
        if (!account_start) break;
        
        char* account_end = account_start;
        int brace_count = 1;
        account_end++;
        
        while (*account_end && brace_count > 0) {
            if (*account_end == '{') brace_count++;
            else if (*account_end == '}') brace_count--;
            account_end++;
        }
        
        if (brace_count > 0) break;
        
        int account_len = account_end - account_start;
        char* account_json = malloc(account_len + 1);
        if (!account_json) break;
        
        strncpy(account_json, account_start, account_len);
        account_json[account_len] = '\0';
        
        char* mint = extract_json_string(account_json, "mint");
        if (mint) {
            strncpy(tokens[*count].mint, mint, sizeof(tokens[*count].mint) - 1);
            tokens[*count].mint[sizeof(tokens[*count].mint) - 1] = '\0';
            free(mint);
            
            tokens[*count].decimals = (int)extract_json_number(account_json, "decimals");
            
            char* amount_str = extract_json_string(account_json, "amount");
            if (amount_str) {
                double raw_amount = atof(amount_str);
                tokens[*count].amount = raw_amount / pow(10, tokens[*count].decimals);
                free(amount_str);
                
                if (strcmp(tokens[*count].mint, "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v") == 0) {
                    strcpy(tokens[*count].symbol, "USDC");
                } else if (strcmp(tokens[*count].mint, "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB") == 0) {
                    strcpy(tokens[*count].symbol, "USDT");
                } else if (strcmp(tokens[*count].mint, "So11111111111111111111111111111111111111112") == 0) {
                    strcpy(tokens[*count].symbol, "wSOL");
                } else {
                    snprintf(tokens[*count].symbol, sizeof(tokens[*count].symbol), 
                             "%.8s", tokens[*count].mint);
                }
                
                (*count)++;
            }
        }
        
        free(account_json);
        pos = account_end;
    }
}

int compare_tokens(const void* a, const void* b) {
    Token* ta = (Token*)a;
    Token* tb = (Token*)b;
    
    if (ta->amount > tb->amount) return -1;
    if (ta->amount < tb->amount) return 1;
    return 0;
}

void print_neofetch_style(const char* wallet, Token* tokens, int token_count, int limit) {
    const char* ascii_art[] = {
        "    ________    ",
        "   /_______/    ",
        "   \\_______\\    ",
        "   /_______/    ",
        "                "
    };
    
    int art_lines = 5;
    int display_count = (limit > 0) ? (limit < token_count ? limit : token_count) : token_count;
    printf("\n");
    printf("%s | Wallet: %s\n", ascii_art[0], wallet);
    for (int i = 1; i < art_lines; i++) {
        if (i - 1 < display_count) {
            Token* token = &tokens[i - 1];
            char amount_str[32];
            if (token->amount >= 1000000) {
                snprintf(amount_str, sizeof(amount_str), "%.2fM", token->amount / 1000000);
            } else if (token->amount >= 1000) {
                snprintf(amount_str, sizeof(amount_str), "%.2fK", token->amount / 1000);
            } else if (token->amount >= 1) {
                snprintf(amount_str, sizeof(amount_str), "%.2f", token->amount);
            } else {
                snprintf(amount_str, sizeof(amount_str), "%.4f", token->amount);
            }
            printf("%s | %-8s | %s\n", 
                   ascii_art[i], 
                   token->symbol, 
                   amount_str);
        } else {
            printf("%s \n", ascii_art[i]);
        }
    }
    
    for (int i = art_lines; i < display_count; i++) {
        Token* token = &tokens[i];
        char amount_str[32];
        if (token->amount >= 1000000) {
            snprintf(amount_str, sizeof(amount_str), "%.2fM", token->amount / 1000000);
        } else if (token->amount >= 1000) {
            snprintf(amount_str, sizeof(amount_str), "%.2fK", token->amount / 1000);
        } else if (token->amount >= 1) {
            snprintf(amount_str, sizeof(amount_str), "%.2f", token->amount);
        } else {
            snprintf(amount_str, sizeof(amount_str), "%.4f", token->amount);
        }
        
        printf("                | %-8s |  %s\n", 
               token->symbol, 
               amount_str);
    }
    
    if (display_count < token_count) {
        printf("                  ...and %d more tokens\n", token_count - display_count);
    }
    
    printf("\n");
}

int main(int argc, char* argv[]) {
    int limit = DEFAULT_LIMIT;
    const char* wallet;
    if (argc == 2) {
        wallet = argv[1];
    } else if (argc == 3 && argv[1][0] == '-') {
        limit = atoi(argv[1] + 1);
        wallet = argv[2];
    } else {
        printf("Usage: %s [-limit] <wallet_address>\n", argv[0]);
        printf("Example: %s -10 <wallet_address>\n", argv[0]);
        return 1;
    }
    const char* rpc_url = "https://api.mainnet-beta.solana.com";
    char sol_json[256];
    snprintf(sol_json, sizeof(sol_json),
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"getBalance\",\"params\":[\"%s\"]}", wallet);
    
    char* sol_response = curl_request(rpc_url, sol_json);
    double sol_balance = 0.0;
    
    if (sol_response) {
        double lamports = extract_json_number(sol_response, "value");
        sol_balance = lamports / 1000000000.0;
        free(sol_response);
    }
    
    char json_data[512];
    snprintf(json_data, sizeof(json_data),
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"getTokenAccountsByOwner\","
        "\"params\":[\"%s\",{\"programId\":\"TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA\"},"
        "{\"encoding\":\"jsonParsed\"}]}", wallet);
    
    char* response = curl_request(rpc_url, json_data);
    if (!response) {
        printf("Error: Failed to fetch token data\n");
        return 1;
    }
    
    Token tokens[MAX_TOKENS];
    int token_count = 0;
    
    if (sol_balance > 0) {
        strcpy(tokens[token_count].mint, "So11111111111111111111111111111111111111112");
        strcpy(tokens[token_count].symbol, "SOL");
        tokens[token_count].amount = sol_balance;
        tokens[token_count].decimals = 9;
        token_count++;
    }
    
    int spl_token_count;
    parse_token_accounts(response, tokens + token_count, &spl_token_count);
    token_count += spl_token_count;
    free(response);
    
    if (token_count == 0) {
        printf("No tokens found for wallet: %s\n", wallet);
        return 1;
    }
    
    qsort(tokens, token_count, sizeof(Token), compare_tokens);
    
    print_neofetch_style(wallet, tokens, token_count, limit);
    
    return 0;
}
