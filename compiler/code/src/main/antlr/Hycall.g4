grammar Hycall;

program:
    (procedure | globalvar_declaration)+;

globalvar_declaration:
    IDENTIFIER (',' IDENTIFIER)* ';'    #globalvar_noval
  | name=IDENTIFIER '=' val=number ';'  #globalvar_num;

procedure:
    PROC IDENTIFIER parameterlistDeclare '{' statement* '}';

parameterlistDeclare:
    '(' (IDENTIFIER (',' IDENTIFIER)*)? ')';


statement:
    FOR '(' IDENTIFIER ':' expression ')' statement   #statement_FOR
  | '{' statement* '}'                                  #statement_BRACE
  | expression ';'                                      #statement_EXPRESSION;

expression:
    list=expression '[' index=expression ']'     #expression_LISTACCESS
  | expression '.' op=(KEY | VAL)            #expression_KEYACCESS
  | op=('+' | '-') expression                    #expression_SIGN
  | expression op=('*' | '/' | '%' ) expression  #expression_MULDIV
  | expression op=('+' | '-') expression         #expression_ADDSUB
  | expression '->' expression                   #expression_KEYVAL
  | IDENTIFIER '=' expression                    #expression_ASSIGN
  | '[' (expression (',' expression)*)? ']'      #expression_LIST
  | '(' expression ')'                           #expression_PARENS
  | IDENTIFIER parameterlistCall                 #expression_PROCCALL
  | IDENTIFIER                                   #expression_VAR
  | number                                       #expression_NUM
  | STRING                                       #expression_STR;


parameterlistCall:
     '(' (expression (',' expression)*)? ')';

number:
    DEC  #number_DECIMAL
  | HEX  #number_HEXADEC
  | BIN  #number_BINARY;


STRING: '"' (~'"')* '"';
DEC: [0-9]+;
HEX: '0x' [0-9a-f]+;
BIN: '0b' [0-1]+;
PROC: 'proc';
FOR: 'for';
KEY: 'key';
VAL: 'val';
IDENTIFIER: [_a-zA-Z][_a-zA-Z0-9]*;
WHITESPACE: [ \t\n\r]+ -> skip;