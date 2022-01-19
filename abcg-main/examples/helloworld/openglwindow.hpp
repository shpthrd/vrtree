#ifndef OPENGLWINDOW_HPP_
#define OPENGLWINDOW_HPP_

#include <random>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "abcg.hpp"


#define M 6
#define m 3
typedef struct rect{
	int x1;
	int y1;
	int x2;
	int y2;

} rect;

typedef struct node{
	int id;
	struct node* child[M+1];
	struct node* parent;
	rect mbr;
	int level;
	int child_n;

} node;

node *root=NULL;//root node, global address

//INSERT FUNCIONS
void Insert(int,int,int,int);//add the rect of the coord params to the root(global)
node** chooseLeaf(node**,rect*);//choose the best node to insertion
void splitNode(node**);//make a split in the node and add the new one to the parent of the original
void adjustTree(node**);//adjust tree see if the node needs an overflow rearrange due to splitNode and do it in the parent if needed
void pickSeeds(node* n,int split[]);//pick seed takes two nodes to make a split, using linear or quadratic search for the two worst pair to be together
node* pickNext(node*);//see where to put the left nodes after pickSeeds()

void search(rect);//search the data of rect if intercect some data in the rtree
int intersect(rect*,rect*);//search if the two rects intercects each other: 1 true 0 false

int isLeaf(node*);//maybe dont need, use some info in the node struct: 1 true 0 false


//AUX FUNCTIONS
node* createEmptyNode();
node* createNode(node*);//create an empty node id is made of global int id_count incrementation
void setMbr(node*,int,int,int,int);
void separator();
void printNode(node*);
void printRect(rect*);
void test(char t[]);
void freeNode(node**);//maybe needs tail recursion
void printTree(node*);//print from root
int areaMbr(rect*,rect*);//calculate the area of the mbr of two rects
int area(rect*);
void adjustNode(node**);//adjust the node param only, used in the splitNode due to changing mbr
void propagateLevel(node*);//when split the root need to grow a level

class OpenGLWindow : public abcg::OpenGLWindow {
 protected:
  void initializeGL() override;
  void paintGL() override;
  void paintUI() override;
  void resizeGL(int width, int height) override;
  void terminateGL() override;

 private:
  GLuint m_vao{};
  GLuint m_vboPositions{};
  GLuint m_vboColors{};
  GLuint m_program{};

  int m_viewportWidth{};
  int m_viewportHeight{};

  std::default_random_engine m_randomEngine;

  int m_delay{200};
  abcg::ElapsedTimer m_elapsedTimer;

  void setupModel(int sides);
};

#endif