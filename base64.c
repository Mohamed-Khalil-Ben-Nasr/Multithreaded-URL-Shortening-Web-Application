const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

void encode(unsigned int n,char* dest) {
    dest[6] = '\0';
		for(int k = 5;k > 0;k--) {
      dest[k] = table[n%64];
      n /= 64;
		}
    dest[0] = table[n];
}

unsigned int charToInt(char ch) {
    if(ch >= 'A' && ch <= 'Z')
        return (int) (ch-'A');
    if(ch >= 'a' && ch <= 'z')
        return 26 + (int) (ch-'a');
    if(ch >= '0' && ch <= '9')
        return 52 + (int) (ch - '0');
    if(ch == '-')
        return 62;
    if(ch == '_')
        return 63;
    return 0;
}

unsigned int decode(char* source) {
    unsigned int n = charToInt(source[0]);
		for(int k = 1;k <= 5;k++) {
      n *= 64;
      n += charToInt(source[k]);
		}
    return n;
}