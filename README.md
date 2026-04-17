# UART-Implementaion-Arduino

Este projeto foi desenvolvido no escopo da disciplina de Sistemas Embarcados, onde foi solicitado que realizassemos a comunicação entre dois Arduinos utilizando interrupções via Timers. Como sugestão foi informado que poderia ser utilizado a biblioteca Serial do Arduino, porém, a abordagem utilizada foi implementar um protocolo semelhante ao protocolo de comunicação UART, tomando como base a estrutura de frame do mesmo.

A estrutura de frame é representada pela seguinte imagem:

![Estrutura_Frame](frame_struct.png)

Como pode ser visto na figura acima, não há bit de correção implementado, dessa forma não é possível que seja implementado correção de erros.

## Execução
Foi utilizado a extensão Platformio do VSCode para compilação e execução do mesmo. Entretando a execução não se restringe somente a esta ferramenta, é possível que se utilize também o Arduino IDE, bastando somente colar o código contido em ``\src\main.cpp`` na plataforma Arduino IDE, e então compilar e enviar o código no arduino.

A configuração de pinos TX e RX é feita no ínicio do código, bastando somente altera os valores ligados as variáveis globais ``TX_PIN`` e ``RX_PIN``. A porta TX pode ser qualquer uma porta capaz de emitir um sinal digital, porém é de extrema importância **certificar-se de que a porta RX tenha a funcionalidade de interrupção externa**. Para verificar os pinos que possuem essa funcionalidade, basta consultar o *datasheet* do Arduino escolhido, ou então consultar a documentação do comando [atachInterrupt](https://docs.arduino.cc/language-reference/en/functions/external-interrupts/attachInterrupt/). Por padrão, como foi utilizado o Mega 2560 utilizamos a porta digital 40 para o TX e 2 para o RX.

## Funcionamento 
O funcionamento é descrito em partes por meio de comentários no próprio código. A sua execução se torna algo simples, bastando somente inserir a palavra a ser enviada na Serial. 

Algo curioso que ocorreu na parte de testes foi a utilização de Arduinos diferentes, esse caminho resulta em uma comunicação incorreta por algum motivo, foi testado utilizando o Arduino Mega 2560 e um Arduino UNO, dessa forma, a transferência de dados não ocorreu da forma esperada. Esse problema afeta diretamente a replicabilidade do projeto.