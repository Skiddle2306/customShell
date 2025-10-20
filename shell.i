%{
int charcount=0, linecount=0;
%}
%%
. {charcount++;}
\n {linecount++; charcount++;}
%%
main() {
yylex();
printf("Lines: %d\n", linecount);
printf("Characters: %d\n", charcount);
}
