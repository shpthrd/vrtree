#include "openglwindow.hpp"

#include <imgui.h>

#include <cppitertools/itertools.hpp>

#include "abcg.hpp"


//GLOBAL VARS

int id_count=0;//counter of ids to assign, later the ids will be used to store the data structure to fast recover(instead of add all the data in the tree again)
int t_c = 0;//counter of test logs used
int free_c = 0;//counter of freed nodes with the function freeNode
int max_size = 1000;//temp size of a bound to the mbr of root and the children
//CLOCK VARS

void OpenGLWindow::initializeGL() {
  const auto *vertexShader{R"gl(
    #version 410

    layout(location = 0) in vec2 inPosition;
    layout(location = 1) in vec4 inColor;

    uniform vec2 translation;
    uniform float scale;

    out vec4 fragColor;

    void main() {
      vec2 newPosition = inPosition * scale + translation;
      gl_Position = vec4(newPosition, 0, 1);
      fragColor = inColor;
    }
  )gl"};

  const auto *fragmentShader{R"gl(
    #version 410

    in vec4 fragColor;

    out vec4 outColor;

    void main() { outColor = fragColor; }
  )gl"};

  // Create shader program
  m_program = createProgramFromString(vertexShader, fragmentShader);

  // Clear window
  abcg::glClearColor(0, 0, 0, 1);
  abcg::glClear(GL_COLOR_BUFFER_BIT);

  // Start pseudo-random number generator
  m_randomEngine.seed(
      std::chrono::steady_clock::now().time_since_epoch().count());
}

void OpenGLWindow::paintGL() {
  // Check whether to render the next polygon
  if (m_elapsedTimer.elapsed() < m_delay / 1000.0) return;
  m_elapsedTimer.restart();

  // Create a regular polygon with a number of sides in the range [3,20]
  std::uniform_int_distribution<int> intDist(3, 20);
  const auto sides{intDist(m_randomEngine)};
  setupModel(sides);

  abcg::glViewport(0, 0, m_viewportWidth, m_viewportHeight);

  abcg::glUseProgram(m_program);

  // Choose a random xy position from (-1,-1) to (1,1)
  std::uniform_real_distribution<float> rd1(-1.0f, 1.0f);
  const glm::vec2 translation{rd1(m_randomEngine), rd1(m_randomEngine)};
  const GLint translationLocation{
      abcg::glGetUniformLocation(m_program, "translation")};
  abcg::glUniform2fv(translationLocation, 1, &translation.x);

  // Choose a random scale factor (1% to 25%)
  std::uniform_real_distribution<float> rd2(0.01f, 0.25f);
  const auto scale{rd2(m_randomEngine)};
  const GLint scaleLocation{abcg::glGetUniformLocation(m_program, "scale")};
  abcg::glUniform1f(scaleLocation, scale);

  // Render
  abcg::glBindVertexArray(m_vao);
  abcg::glDrawArrays(GL_TRIANGLE_FAN, 0, sides + 2);
  abcg::glBindVertexArray(0);

  abcg::glUseProgram(0);
}

void OpenGLWindow::paintUI() {
  abcg::OpenGLWindow::paintUI();

  {
    const auto widgetSize{ImVec2(200, 72)};
    ImGui::SetNextWindowPos(ImVec2(m_viewportWidth - widgetSize.x - 5,
                                   m_viewportHeight - widgetSize.y - 5));
    ImGui::SetNextWindowSize(widgetSize);
    const auto windowFlags{ImGuiWindowFlags_NoResize |
                           ImGuiWindowFlags_NoCollapse |
                           ImGuiWindowFlags_NoTitleBar};
    ImGui::Begin(" ", nullptr, windowFlags);

    ImGui::PushItemWidth(140);
    ImGui::SliderInt("Delay", &m_delay, 0, 200, "%d ms");
    ImGui::PopItemWidth();

    if (ImGui::Button("Clear window", ImVec2(-1, 30))) {
      abcg::glClear(GL_COLOR_BUFFER_BIT);
    }

    ImGui::End();
  }
}

void OpenGLWindow::resizeGL(int width, int height) {
  m_viewportWidth = width;
  m_viewportHeight = height;

  abcg::glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGLWindow::terminateGL() {
  abcg::glDeleteProgram(m_program);
  abcg::glDeleteBuffers(1, &m_vboPositions);
  abcg::glDeleteBuffers(1, &m_vboColors);
  abcg::glDeleteVertexArrays(1, &m_vao);
}

void OpenGLWindow::setupModel(int sides) {
  // Release previous resources, if any
  abcg::glDeleteBuffers(1, &m_vboPositions);
  abcg::glDeleteBuffers(1, &m_vboColors);
  abcg::glDeleteVertexArrays(1, &m_vao);

  // Select random colors for the radial gradient
  std::uniform_real_distribution<float> rd(0.0f, 1.0f);
  const glm::vec3 color1{rd(m_randomEngine), rd(m_randomEngine),
                         rd(m_randomEngine)};
  const glm::vec3 color2{rd(m_randomEngine), rd(m_randomEngine),
                         rd(m_randomEngine)};

  // Minimum number of sides is 3
  sides = std::max(3, sides);

  std::vector<glm::vec2> positions(0);
  std::vector<glm::vec3> colors(0);

  // Polygon center
  positions.emplace_back(0, 0);
  colors.push_back(color1);

  // Border vertices
  const auto step{M_PI * 2 / sides};
  for (const auto angle : iter::range(0.0, M_PI * 2, step)) {
    positions.emplace_back(std::cos(angle), std::sin(angle));
    colors.push_back(color2);
  }

  // Duplicate second vertex
  positions.push_back(positions.at(1));
  colors.push_back(color2);

  // Generate VBO of positions
  abcg::glGenBuffers(1, &m_vboPositions);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_vboPositions);
  abcg::glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(glm::vec2),
                     positions.data(), GL_STATIC_DRAW);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);

  // Generate VBO of colors
  abcg::glGenBuffers(1, &m_vboColors);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_vboColors);
  abcg::glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3),
                     colors.data(), GL_STATIC_DRAW);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);

  // Get location of attributes in the program
  const auto positionAttribute{
      abcg::glGetAttribLocation(m_program, "inPosition")};
  const auto colorAttribute{abcg::glGetAttribLocation(m_program, "inColor")};

  // Create VAO
  abcg::glGenVertexArrays(1, &m_vao);

  // Bind vertex attributes to current VAO
  abcg::glBindVertexArray(m_vao);

  abcg::glEnableVertexAttribArray(positionAttribute);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_vboPositions);
  abcg::glVertexAttribPointer(positionAttribute, 2, GL_FLOAT, GL_FALSE, 0,
                              nullptr);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);

  abcg::glEnableVertexAttribArray(colorAttribute);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, m_vboColors);
  abcg::glVertexAttribPointer(colorAttribute, 3, GL_FLOAT, GL_FALSE, 0,
                              nullptr);
  abcg::glBindBuffer(GL_ARRAY_BUFFER, 0);

  // End of binding to current VAO
  abcg::glBindVertexArray(0);
}


//===============================================================================================
//AUX FUNCIONS
//===============================================================================================
node* createEmptyNode(){
	//test("create node");
	node* n = (node *) malloc(sizeof(node));
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
	node* n = (node *) malloc(sizeof(node));
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