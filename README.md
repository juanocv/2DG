# 2DG

Autoria:
> Diego Guerra\ RA: 11201810534\
> Juan Oliveira de Carvalho\ RA: 11201810997

## Objetivos
O objetivo deste projeto é desenvolver uma aplicação simples com gráficos 2D, utilizando-se de técnicas e bibliotecas de computação gráfica.

## Motivação

Este projeto foi desenvolvido tendo em mente qualquer pessoa estudando a área de grafos, visando fornecer uma visualização gráfica de grafos aleatórios de forma simples e rápida.

## Live demo

Uma versão compilada em WebAssembly está disponível para ser executada em qualquer navegador web neste [link](https://juanocv.github.io/2DG/graph/).

Também é possível compilar o projeto localmente, seguindo as instruções de [instalação do ABCg](https://hbatagelo.github.io/cg/config.html), clonando o repositório e executando os scripts
```build.sh``` ou ```build.bat```, caso esteja no Linux ou Windows respectivamente. Os scripts irão compilar o projeto e criar um executável no diretório ```build```.

TO-DO: adicionar print do projeto rodando

## Funcionalidades

É possível gerar tanto grafos **conectados** quanto **desconectados**, além de:
-  Determinar a quantidade de nós (de 1 a 10 nós)
-  Personalizar a cor de exibição dos nós
-  Escolher o raio das arestas exibidas

Também são exibidas algumas características do grafo gerado, como:
-  Grau de cada nó
-  Lista de adjascências dos nós
-  Tipo de grado (conectado/ não conectado)
-  Total de nós
-  Total de arestas
-  Grau médio do grafo

## Implementação
A aplicação foi implementada utilizando a biblioteca [ABCg](https://github.com/hbatagelo/abcg), desenvolvida pelo professor Harlen Batagelo para o curso de Computação Gráfica na UFABC, e o pipeline gráfico do OpenGL.

TO-DO: adicionar detalhes da implementação
