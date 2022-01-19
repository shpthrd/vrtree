#include <stdio.h>
#include <stdlib.h>
#include <time.h>


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

//GLOBAL VARS
node *root=NULL;//root node, global address
int id_count=0;//counter of ids to assign, later the ids will be used to store the data structure to fast recover(instead of add all the data in the tree again)
int t_c = 0;//counter of test logs used
int free_c = 0;//counter of freed nodes with the function freeNode
int max_size = 1000;//temp size of a bound to the mbr of root and the children
//CLOCK VARS
clock_t start, end;
double cpu_time_used;

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

//===============================================================================================
//MAIN FUNCTION
//===============================================================================================
int main(int argc, char *argv[]){
	printf("program name: %s\n",argv[0]);
	long k = 10;
	if(argc==1){
		printf("no parameter, so k=10\n");
	}
	else{
		k = strtol(argv[1], NULL, 10);
		printf("other argument so k = %ld\n",k);
	}
	start = clock();
	root= createNode(NULL);
	//printTree(root);
	//printf("\n");
	int i;
	for(i=0;i<k;i++){
		Insert(i,i,i+1,i+1);
	}
	
	//printTree(root);
	end = clock();
	cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
	printf("time elapsed: %f\n",cpu_time_used);
	freeNode(&root);
	return 0;
}


//===============================================================================================
//AUX FUNCIONS
//===============================================================================================
node* createEmptyNode(){
	//test("create node");
	node* n = malloc(sizeof(node));
	n->id = id_count++;
	n->level = 0;
	n->child_n = 0;
	int i;
	for(i = 0; i < M+1; i++){
		n->child[i] = NULL;
	}
	n->mbr.x1 = n->mbr.x2 = n->mbr.y1 = n->mbr.y2  = 0;
	return n;	
}

node* createNode(node* parent){
	//printf("createNode %d\n",id_count);
	node* n = malloc(sizeof(node));
	n->id = id_count++;
	if(parent == NULL){
		n->level = 0;
	}
	else{
		n->level = (parent)->level+1;
		n->parent = parent;
	}
	n->child_n = 0;
	int i;
	for(i = 0; i < M+1; i++){
		n->child[i] = NULL;
	}
	n->mbr.x1 = n->mbr.x2 = n->mbr.y1 = n->mbr.y2  = 0;
	return n;	
}
void setMbr(node* node,int x1, int y1, int x2, int y2){
	node->mbr.x1 = x1;
	node->mbr.y1 = y1;
	node->mbr.x2 = x2;
	node->mbr.y2 = y2;
}

void separator(){
	printf("\n===========================================================\n\n");
}
void printNode(node* n){
	//test("begin printnode");
	if(n == NULL){
		printf("node null\n");
		return;
	}
	printf("node id : %d\nnode level : %d\nadr: %p\ndad: %p\n",n->id,n->level,n,n->parent);
	printRect(&n->mbr);
	int i=0;
	while(i<M+1 && n->child[i] != NULL){
		printf("child[%d] id: %d\n",i, n->child[i]->id);
		i++;
	}
	printf("--\n");

}
void printRect(rect* rect){
	printf("(%d,%d)(%d,%d)\n",rect->x1,rect->y1,rect->x2,rect->y2);
}
void freeNode(node** node){
	if(node == NULL) return;
	int i = 0;
	for(i = 0; i < M+1; i++){
		if((*node)->child[i] != NULL)
			freeNode(&(*node)->child[i]);
	}
	free(*node);
	*node = NULL;
	free_c++;
}
int area(rect* r){
	if(r == NULL) return -1;
	return (r->x2-r->x1)*(r->y2-r->y1);
}

/* void printTree(node* n){
	int i;
	
	for(i = 0;i<n->level;i++){
		printf("\t");
	}
	printf("id:%d-lv:%d-(%d,%d)(%d,%d)-adr:%p-dad:%p\n",n->id,n->level,n->mbr.x1,n->mbr.y1,n->mbr.x2,n->mbr.y2,n,n->parent);
	i=0;
	while(n->child[i] != NULL && i<=M){
			printTree(n->child[i]);
			i++;
	}
} */
void printTree(node* n){
	int i;
	
	for(i = 0;i<n->level;i++){
		printf("\t");
	}
	printf("id:%d-lv:%d-(%d,%d)(%d,%d)\n",n->id,n->level,n->mbr.x1,n->mbr.y1,n->mbr.x2,n->mbr.y2);
	i=0;
	while(n->child[i] != NULL && i<=M){
			printTree(n->child[i]);
			i++;
	}
}

int areaMbr(rect* r1,rect* r2){
	if(r1 == NULL || r2 == NULL) return -1;
	//printf("dentro do areaMbr\n");
	int x1,x2,y1,y2;
	if(r1->x1 < r2->x1) x1 = r1->x1;
	else x1 = r2->x1;
	if(r1->x2 > r2->x2) x2 = r1->x2;
	else x2 = r2->x2;
	if(r1->y2 > r2->y2) y2 = r1->y2;
	else y2 = r2->y2;
	if(r1->y1 < r2->y1) y1 = r1->y1;
	else y1 = r2->y1;
	//printf("area: %d\n",(y2-y1)*(x2-x1));
	return (y2-y1)*(x2-x1);
}

void adjustNode(node** n){
	if(n == NULL)return;
	if(*n == NULL)return;
	if((*n)->child[0] == NULL)return;
	//printf("init of adjustNode\n");
	int i=1;
	rect aux = (*n)->child[0]->mbr;//this is a copy? 
	(*n)->child[0]->parent = *n;
	while(i<M && (*n)->child[i] != NULL){
		if((*n)->child[i]->mbr.x1 < aux.x1){
			aux.x1 = (*n)->child[i]->mbr.x1;
		}
		if((*n)->child[i]->mbr.y1 < aux.y1){
			aux.y1 = (*n)->child[i]->mbr.y1;
		}
		if((*n)->child[i]->mbr.x2 > aux.x2){
			aux.x2 = (*n)->child[i]->mbr.x2;
		}
		if((*n)->child[i]->mbr.y2 > aux.y2){
			aux.y2 = (*n)->child[i]->mbr.y2;
		}
		(*n)->child[i]->parent = *n;
		i++;
	}
	(*n)->mbr = aux;//is this a copy??
	
	
}

void propagateLevel(node* n){
	if(n == NULL) return;
	int i=0;
	while(n->child[i] != NULL){
		n->child[i]->level = n->child[i]->level + 1;
		propagateLevel(n->child[i]);
		i++;
	}
}

//===============================================================================================
//INSERT FUNCIONS
//===============================================================================================

void Insert(int x1, int y1, int x2, int y2){
	//printf("inserting the data: %d,%d,%d,%d\n",x1,y1,x2,y2);
	node* data = createEmptyNode();
	setMbr(data,x1,y1,x2,y2);
	node** selected = chooseLeaf(&root,&data->mbr);
	int i = 0;
	while((*selected)->child[i]!=NULL){
		if(i>=M+1){
			printf("erro insertion on M+1 node\n");
			return;
		}
		i++;
	}
	(*selected)->child[i]=data;
	data->parent=(*selected);
	data->level = (*selected)->level+1;
	//printTree(root);
	adjustTree(selected);
	//printTree(root);
	//printf("data added\n");

}
node** chooseLeaf(node** n, rect* dataRect){
	if(n == NULL) return NULL;
	if(*n == NULL) return NULL;
	if(root == *n && (root)->child[0]==NULL) return &root;//this means root is leaf and has zero data entries
	if((*n)->child[0]->child[0]==NULL) {
		//printf("id of selected node: %d\n",(*n)->id);
		return n;//this means this node has a child and this child doesnt has a child, which means it is data, wich menas the node is leaf
	}
	//if is not leaf need to find the least increasing node to add the data
	int area_diff=-1;
	int i = 0, node_choosen = 0;
	while(i < M && (*n)->child[i] != NULL){//run until M-1, this node doesnt supose to has a M+1 child(child[M])
		//calcular area e comparar com area_diff
		int area_calc = areaMbr(&(*n)->child[i]->mbr,dataRect) - area(&(*n)->child[i]->mbr);//test if can get negative, but probably doesnt
		if(area_diff == -1){
			area_diff = area_calc;

		}
		else{
			if(area_calc < area_diff){
				area_diff = area_calc;
				node_choosen = i;
			}
		}
		i++;
	}
	return chooseLeaf(&((*n)->child[node_choosen]),dataRect);
}

void splitNode(node** n){ //it is possible that this function needs a pointer of pointer aproach //CRITICAL//CRITICAL//CRITICAL
	if (n == NULL) return;
	if(*n == NULL) return;
	if((*n)->child[M] == NULL) return;//this means the node doesnt have M+1 children
	int i;
	int split[2];
	pickSeeds(*n,split);//CRITICAL
	//printf("inside splitNode seeds: %d,%d\n",split[0],split[1]);
	//printf("**test split [0] and [1]: %d,%d\n",split[0],split[1]);
	//printf("*n->id: %d\n",(*n)->id);
	int max = m+1;
	node* n1 = createNode((*n)->parent);
	node* n2 = createNode((*n)->parent);
	n1->child[0] = (*n)->child[split[0]];
	n2->child[0] = (*n)->child[split[1]];
	int i_n1=1,i_n2=1;
	i=0;
	while((*n)->child[i]!= NULL && i < M+1){
		if(i != split[0] && i != split[1]){
			//printf("entrou no if com o indice: %d, i_n1: %d, i_n2: %d\n",i,i_n1,i_n2);
			if(i_n1 < max){
				if(areaMbr(&n1->child[0]->mbr,&(*n)->child[i]->mbr) < areaMbr(&n2->child[0]->mbr,&(*n)->child[i]->mbr) || i_n2 >= max-1){
					n1->child[i_n1] = (*n)->child[i];
					i_n1++;
				}
				else{
					n2->child[i_n2] = (*n)->child[i];
					i_n2++;
				}
			}
			else{
				//inserir no n2
				n2->child[i_n2] = (*n)->child[i];
				i_n2++;
			}
		}
		i++;
	}
	//printNode(n1);
	//printNode(n2);
	if(*n == root){//AQUI FALTA DAR FREE EM ALGUMA COISA?
		//printf("split root\n");
		free(root);
		root= createEmptyNode();
		(root)->id = 0;
		(root)->child[0] = n1;
		(root)->child[1] = n2;
		n1->parent = root;
		n2->parent = root;
		propagateLevel(root);
		//printf("end of n == root\n");
	}
	else{
		//printf("split node\n");
		i=0;
		//printf("## printing root:\n");
		//printNode(root);
		if((*n)->parent == root){
			while(i<M && root->child[i] != NULL){
				if(root->child[i]== *n){
					free(*n);
					root->child[i] = n1;
				}
				i++;
			}
			root->child[i] = n2;
		}
		else{
			while(i<M && n1->parent->child[i] != NULL){
				if(n1->parent->child[i] == *n){
					//printf("n1 index: %d,n->id: %d n1->id:%d n->dad: %p n1->dad: %p\n",i,(*n)->id,n1->id,(*n)->parent,n1->parent);
					free(*n);
					n1->parent->child[i] = n1;//pode ser que dê bosta, mas acho que não
					//printf("n1 index: %d,n->id: %d n1->id:%d n->dad: %p n1->dad: %p\n",i,(*n)->id,n1->id,(*n)->parent,n1->parent);
				}
				i++;
			}
			//printf("n2 index: %d\n",i);
			n1->parent->child[i] = n2;
			//printf("## printing n:\n");
			//printNode(*n);
		}
		
		
		
	}
	//printTree(root);
	adjustNode(&n1);
	adjustNode(&n2);
	//printf("## printing root:\n");
	//printNode(root);
}

void adjustTree(node** n){
	//maybe adjust need to pass the child that will make the adjust, but need to see if need to adjust if split but i guess will not
	if(n == NULL)return;
	if((*n)->child[0] == NULL)return;
	//printf("init of adjustTree\n");
	//printf("printing n: \n");
	//printNode(*n);
	if((*n)->child[M] != NULL){
		//printf("\nadj->split node: %d adr: %p\n\n",(*n)->id,(*n));
		splitNode(n);
		//printf("\nend of adj->split node: %d\n\n",(*n)->id);
		//printTree(root);
	}
	int i=1;
	rect aux = (*n)->child[0]->mbr;//this is a copy? 
	while(i<M && (*n)->child[i] != NULL){
		if((*n)->child[i]->mbr.x1 < aux.x1){
			aux.x1 = (*n)->child[i]->mbr.x1;
		}
		if((*n)->child[i]->mbr.y1 < aux.y1){
			aux.y1 = (*n)->child[i]->mbr.y1;
		}
		if((*n)->child[i]->mbr.x2 > aux.x2){
			aux.x2 = (*n)->child[i]->mbr.x2;
		}
		if((*n)->child[i]->mbr.y2 > aux.y2){
			aux.y2 = (*n)->child[i]->mbr.y2;
		}
		i++;
	}
	(*n)->mbr = aux;//is this a copy??
	//printf("\n\n");
	//printTree(*n);
	//printf("AFTER ADJUST\n");
	if((*n) != root){
		adjustTree(&(*n)->parent);
	}

}
void pickSeeds(node* n,int split[]){
	if(n == NULL) return;
	//printf("init of pickSeeds\n");
	int i, i1 = 0, i2 = 1;
	if(areaMbr(&n->child[0]->mbr,&n->child[1]->mbr) > areaMbr(&n->child[0]->mbr,&n->child[2]->mbr)){
		if(areaMbr(&n->child[0]->mbr,&n->child[1]->mbr) > areaMbr(&n->child[1]->mbr,&n->child[2]->mbr)){
			i1=0;
			i2=1;
		}
		else{
			i1=1;
			i2=2;
		}
	}
	else{
		if(areaMbr(&n->child[0]->mbr,&n->child[2]->mbr) > areaMbr(&n->child[1]->mbr,&n->child[2]->mbr)){
			i1=0;
			i2=2;
		}
		else{
			i1=1;
			i2=2;
		}
	}
	int area = areaMbr(&n->child[i1]->mbr,&n->child[i2]->mbr);
	for(i=3;i<M+1;i++){
		int a1 = areaMbr(&n->child[i1]->mbr,&n->child[i]->mbr);
		int a2 = areaMbr(&n->child[i2]->mbr,&n->child[i]->mbr);
		if(a1>a2){
			if(a1>area){
				i2=i;
				area = areaMbr(&n->child[i1]->mbr,&n->child[i2]->mbr);
			}
		}
		else{
			if(a2>area){
				i1=i;
				area = areaMbr(&n->child[i1]->mbr,&n->child[i2]->mbr);
			}
		}
		
	}
	split[0] = i1;
	split[1] = i2;
	//printf("inside pickSeed seeds: %d,%d\n",split[0],split[1]);
}