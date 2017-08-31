#include <stdio.h>
#include <string.h>
#include <time.h>

int main() {
    FILE *fp_in;
    FILE *fp_out;
    FILE *fp_time;
    char ch;
    char line[100];
    fp_in = fopen("r8204.rdb.parse", "r");
    fp_out = fopen("keys_log.txt", "w");
    fp_time = fopen("cost_time.txt", "w");

    while (!feof(fp_in)) {
        fgets(line, 100, fp_in);  //读取一行
        if (strlen(line) < 2) {
            continue;
        }
        fputs(line, fp_out);

        if (line[strlen(line) - 1] != '\n') {
            while ((ch = fgetc(fp_in)) != '\n') { ;
            }
            fputc('\n', fp_out);
        }
    }
    fclose(fp_in);
    fclose(fp_out);

    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    fputs(asctime (timeinfo), fp_time);
}