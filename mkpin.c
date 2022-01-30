#include <string.h>
#include <sodium.h>

int main() {
    char pin[25];
    char hashed[crypto_pwhash_STRBYTES];

    if (sodium_init() == -1) {
        return 1;
    }

    fprintf(stderr, "Type PIN on standard input: ");

    fgets(pin, sizeof pin, stdin);
    char *p = strchr(pin, '\n');
    if(p) *p = '\0';

    crypto_pwhash_str(hashed,
		      pin, strlen(pin),
		      crypto_pwhash_OPSLIMIT_MODERATE,
		      crypto_pwhash_MEMLIMIT_MODERATE);
    puts(hashed);
}
