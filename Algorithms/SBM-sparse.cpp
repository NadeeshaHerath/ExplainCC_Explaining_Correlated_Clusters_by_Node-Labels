#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <getopt.h>
#include <vector>
#include <map>
#include <limits>
#include <iostream>
#include "queue.h"


#define TAILQ_REATTACH(dst, src, field) do {                  \
    (dst)->tqh_first = (src)->tqh_first;                      \
	if ((dst)->tqh_first != NULL) {                           \
		(dst)->tqh_first->field.tqe_prev = &(dst)->tqh_first; \
		(dst)->tqh_last = (src)->tqh_last;                    \
	}                                                         \
	else                                                      \
		(dst)->tqh_last = &(dst)->tqh_first;                  \
} while (/*CONSTCOND*/0)


TAILQ_HEAD(linkhead, elink);
TAILQ_HEAD(labelhead, labelink); 
TAILQ_HEAD(nodehead, node); 
TAILQ_HEAD(infohead, label);
TAILQ_HEAD(leafhead, leaf);

struct edge;
struct label;
struct labelink;
struct leaf;
struct node;

typedef std::vector<node> nodevector;
typedef std::vector<edge> edgevector;
typedef std::vector<labelink> linkvector;
typedef std::vector<label> labelvector;

typedef std::map<uint32_t, uint32_t> uintmap;
typedef std::map<uint32_t, label *> labelmap;



struct elink{
    edge *e;
    TAILQ_ENTRY(elink) entries;
};

/*************. Node.   ***************/
struct node{
    uint32_t name;
    uint32_t deg,ldeg;

    bool haslabel;
    bool active;

    linkhead adj;
    labelhead labels;

    TAILQ_ENTRY(node) entries;

    edge *first() {elink *l=TAILQ_FIRST(&adj); if (l) return l->e; else return 0;};

};

/****************** Edge **************/

struct edge{
    node *u[2];
    elink links[2];

    bool is_positive; 

    void detach(){
        TAILQ_REMOVE(&u[0]->adj, &links[0], entries);
        TAILQ_REMOVE(&u[1]->adj, &links[1], entries);

        u[0]-> deg--;
        u[1]-> deg--;
    }

    void attach (node *a, node*b,bool sign){
        u[0] = a;
        u[1] = b;
        is_positive = sign;

        a->deg++;
        b->deg++;

        links[0].e = this;
        links[1].e = this;

        TAILQ_INSERT_TAIL(&a->adj, &links[0], entries);
        TAILQ_INSERT_TAIL(&b->adj, &links[1], entries);

    }

    edge *next(node*a){
        elink *l=0;
        if (u[0]==a) l=TAILQ_NEXT(&links[0], entries);
        if (u[1]==a) l=TAILQ_NEXT(&links[1], entries);
        if (l) return l->e;
        return 0;

    }

    node* other(node*a){
        return u[u[0]==a];
    }

};

/********************* Label ***************/
struct label{
    uint32_t name;
    labelhead nodes;
    uint32_t nodecnt;
    
	uint32_t crosscnt;
    uint32_t crosscnt_pos;
	uint32_t crosscnt_neg;

    label *leech;

    void init(uint32_t n){
        name = n;
        TAILQ_INIT(&nodes);
        nodecnt=crosscnt=crosscnt_pos=crosscnt_neg=0;
        leech =0;
    }

    TAILQ_ENTRY(label) entries;

};

/********************* Label Link ****************/
struct labelink{
    node *u;
    label *l;

    void attach(node*a, label*b){
        u=a;
        l=b;
        u->ldeg++;
        l->nodecnt++;
        TAILQ_INSERT_TAIL(&l->nodes, this, entries);
        TAILQ_INSERT_TAIL(&u->labels, this, nentries);
    }

    void
	detach()
	{
        u->ldeg--;
		l->nodecnt--;
		TAILQ_REMOVE(&u->labels, this, nentries);
		TAILQ_REMOVE(&l->nodes, this, entries);
	}

	void
	reassign(label *b)
	{
		TAILQ_REMOVE(&l->nodes, this, entries);
		l->nodecnt--;
		l = b;
        this->l = b; 
		TAILQ_INSERT_TAIL(&l->nodes, this, entries);
		l->nodecnt++;
	}


	TAILQ_ENTRY(labelink) entries, nentries;
};


/***************** Leaf *****************/

struct leaf{
    leaf(){
        index =0;
        nodecnt=0;
        burden=0;
		
        
        TAILQ_INIT(&nodes);
        TAILQ_INIT(&labels);
		left_child = nullptr;
		right_child = nullptr;
		split_label = -1;

    }
    nodehead nodes;
    infohead labels;
    uint32_t index;
    uint32_t nodecnt;
    double burden;

	leaf *left_child;
	leaf *right_child;
	int split_label;
    
    TAILQ_ENTRY(leaf) entries;

};

/****************** Action **********/
struct action{
    leaf *l;
    label *lab;

    double score;
    bool isright;
    bool do_split;
};


/**************** Graph *****************/
struct graph {
	nodevector nodes;
	edgevector edges;
	linkvector links;

	leafhead leaves;

	uintmap nodenames;
	labelmap labelnames;

	double lambda;
	uint32_t total_negative_edges =0;
};


/*********************** splitleaves ****************/

void splitleaves(nodehead *src, labelhead *labelled, nodehead *right, nodehead *left)
{
	
	TAILQ_REATTACH(left, src, entries);
    
	labelink *l;
	TAILQ_FOREACH(l, labelled, entries) {
		l->u->haslabel = true;
		TAILQ_REMOVE(left, l->u, entries);
		TAILQ_INSERT_TAIL(right, l->u, entries);
	}   
}


/*********************** count burden ****************/
void countburden(leaf *right, leaf *left, const leaf *src){
	
    right->burden =0;
	double crosscnt = 0;

	node *u;
	TAILQ_FOREACH(u, &right->nodes, entries) {
		right->burden += u->ldeg;
		for (edge *e = u->first(); e; e = e->next(u)) {
			node *v = e->other(u);
            assert(v);
            if (!v) continue;
			if (u->haslabel != v->haslabel) {
					crosscnt += u->ldeg + v->ldeg;
			}
			else{
					right->burden +=u->ldeg;
			}
			
				
		}
	}

	if (left) {
		assert(right->burden <= src->burden);
		left->burden = src->burden - crosscnt - right->burden;
	}

}

/*************** count edges *******************/
void countedges(label *right, label *left) {
	
	right->crosscnt_pos = 0;
	right->crosscnt_neg=0;

	labelink *l;
	TAILQ_FOREACH(l, &right->nodes, entries) {
		l->u->active = true;
	}

	TAILQ_FOREACH(l, &right->nodes, entries) {
		node *u = l->u;

		for (edge *e = u->first(); e; e = e->next(u)) {
			node *v = e->other(u);
			if (v->haslabel != u->haslabel) continue;
			if (u->active != v->active) {
				if(e->is_positive){
					right->crosscnt_pos++;
				}
				else{
					right->crosscnt_neg++;
				}
			}
		}

    }
	if (left) {
		left->crosscnt_pos -= right->crosscnt_pos;
		left->crosscnt_neg -= right->crosscnt_neg;
	}

	TAILQ_FOREACH(l, &right->nodes, entries) {
		l->u->active = false;
	}


}



/*************** cross edges - leaf *******************/
void countcrossedges(leaf & lf)
{
	
	node *u;
	TAILQ_FOREACH(u, &lf.nodes, entries) {
		edge *e, *enext;
		for (e = u->first(); e; e = enext) {
			enext = e->next(u);
			node *v = e->other(u);
			if (u->haslabel == v->haslabel) continue;
			e->detach();

			labelink *l1 = TAILQ_FIRST(&u->labels);
			labelink *l2 = TAILQ_FIRST(&v->labels);

			// Do merge iteration
			while (l1 || l2) {
				if (l2 == 0 || (l1 && l1->l->name < l2->l->name)) {
					if (e->is_positive) {
						l1->l->leech->crosscnt_pos--;
					}
					else{
						l1->l->leech->crosscnt_neg--;
					}
					
					l1 = TAILQ_NEXT(l1, nentries);
				}
				
				else if (l1 == 0 || (l2 && l1->l->name > l2->l->name)) {
					if (e->is_positive){
						l2->l->crosscnt_pos--;
					}
					else{
						l2->l->crosscnt_neg--;
					}
						
					l2 = TAILQ_NEXT(l2, nentries);
				
				}
				else {
					l1 = TAILQ_NEXT(l1, nentries);
					l2 = TAILQ_NEXT(l2, nentries);
				}
			}
		}
	}
}


/*************** split label nodes *******************/
void splitlabelnodes(leaf &right, leaf &left, leaf &src)
{
	TAILQ_REATTACH(&left.labels, &src.labels, entries);

	node *u;
	TAILQ_FOREACH(u, &right.nodes, entries) {
		labelink *l;
		TAILQ_FOREACH(l, &u->labels, nentries) {
			label *info = l->l;
			if (info->leech == 0) {
				label *lab = new label;
				lab->init(info->name);
				info->leech = lab;
				lab->leech = info;

				TAILQ_INSERT_TAIL(&right.labels, lab, entries);
			}
			l->reassign(info->leech);
		}
	}


	label *il;
	TAILQ_FOREACH(il, &right.labels, entries) {
		label *ol = il->leech;
		countedges(il, ol);
	}

    countcrossedges(right);

	// Clean up pointers for the next use
	TAILQ_FOREACH(il, &right.labels, entries) {
		assert(il->leech);
		label *ol = il->leech;
		ol->leech = 0;
		il->leech = 0;
	}

}


/**************** Delete Redundant Labels *******************/
void delete_redundant_labels(leaf &lf) {
    label *lab = TAILQ_FIRST(&lf.labels);
    while (lab) {
        label *next = TAILQ_NEXT(lab, entries);  

        if (lab->nodecnt == 0 || lab->nodecnt == lf.nodecnt) {
            labelink *lk;
            while ((lk = TAILQ_FIRST(&lab->nodes))) {
                lk->detach();   
            }
            TAILQ_REMOVE(&lf.labels, lab, entries);
            delete lab;
        }

        lab = next;  
    }
}



/*************** split *******************/
void split(action &a, leaf &right, leaf &left)
{
	leaf & src = *a.l;
	bool isright = a.isright;
	label & info = *a.lab;

	TAILQ_INIT(&right.nodes);
	TAILQ_INIT(&left.nodes);

	
	right.nodecnt = info.nodecnt;
	left.nodecnt = src.nodecnt - info.nodecnt;
	splitleaves(&src.nodes, &info.nodes, &right.nodes, &left.nodes);
	countburden(&right, &left, &src);
	
    if (right.burden < left.burden) {
		splitlabelnodes(right, left, src);
	}
	else {
		splitlabelnodes(left, right, src);
	}
  

	delete_redundant_labels(right);
	delete_redundant_labels(left);

	node *u;
	TAILQ_FOREACH(u, &right.nodes, entries) {
		u->haslabel = false;
	}
}
	


/**************** Split score ************/
double split_score(leaf *lf, label *lab, graph &g) {
	double lambda = g.lambda;
	double r_pos = lab->crosscnt_pos;
	double r_neg = lab->crosscnt_neg;
  
    return (1-lambda)*r_neg-lambda*r_pos;
}


action findbest(leaf *l, graph &g) {
    action best;
    best.score = -std::numeric_limits<double>::infinity();
    best.l = l;
    best.lab = nullptr;
    best.do_split = false;

    if (!l) return best;

    label *lab;
    TAILQ_FOREACH(lab, &l->labels, entries) {
        double score = split_score(l, lab, g);
        if (score > best.score) {
            best.score = score;
            best.lab = lab;
        }
    }

    best.do_split = (best.lab != nullptr && best.score > 0);
    return best;
}

/*************** Apply *******************/
void apply(action &a, leafhead *leaves, uint32_t index)
{
	leaf *right = new leaf;
	leaf *left = new leaf;
    uint32_t split_id = a.lab->name;
	split(a, *right, *left);

	right->index = index;
	left->index = index + 1;

	a.l->left_child = left;
    a.l->right_child = right;
    a.l->split_label = split_id; 
    

	TAILQ_INSERT_AFTER(leaves, a.l, right, entries);
	TAILQ_INSERT_AFTER(leaves, right, left, entries);
	TAILQ_REMOVE(leaves, a.l, entries);
	
}


/*************** Expand *******************/
void expand(leaf *l, graph &g, uint32_t &index, FILE *tf) {
    action a = findbest(l, g);

    if (!a.do_split) return;
    apply(a, &g.leaves, index);
    index += 2; 
    fprintf(tf, "%d %d %f %d %d\n", a.l->index, a.l->split_label, -a.score, a.l->left_child->index, a.l->right_child->index);

    expand(a.l->left_child, g, index, tf);
    expand(a.l->right_child, g, index, tf);
}


/************** read sizes **********/
void read_sizes(graph & g, FILE *lf, FILE *posf, FILE *negf)
{
	uint32_t a, b;
	uint32_t cnt, ecnt;

	cnt = 0;
	ecnt = 0;

    /******* Label File  *********/
	while (fscanf(lf, "%d%d", &a, &b) == 2) {
		if (g.nodenames.count(a) == 0)
			g.nodenames[a] = cnt++;

		if (g.labelnames.count(b) == 0)
			g.labelnames[b] = 0;

		ecnt++; 
	}

	g.links.resize(ecnt);

    /******* Edge File  *********/
	ecnt = 0;

    /****** Positive Edges ******/
	while (fscanf(posf, "%d%d", &a, &b) == 2) {
		if (a == b) continue;
		if (g.nodenames.count(a) == 0)
			g.nodenames[a] = cnt++;
		if (g.nodenames.count(b) == 0)
			g.nodenames[b] = cnt++;
		ecnt++;
	}

    /****** Negative Edges ******/
	while (fscanf(negf, "%d%d", &a, &b) == 2) {
		if (a == b) continue;
		if (g.nodenames.count(a) == 0)
			g.nodenames[a] = cnt++;
		if (g.nodenames.count(b) == 0)
			g.nodenames[b] = cnt++;
		ecnt++;
	}

	g.edges.resize(ecnt);
	g.nodes.resize(cnt);

	TAILQ_INIT(&g.leaves);

	leaf *root = new leaf;
	TAILQ_INSERT_TAIL(&g.leaves, root, entries);

	root->nodecnt = cnt;
	TAILQ_INIT(&root->nodes);
	TAILQ_INIT(&root->labels);

	for (uint32_t i = 0; i < g.nodes.size(); i++) {
		TAILQ_INIT(&g.nodes[i].adj);
		TAILQ_INIT(&g.nodes[i].labels);
		g.nodes[i].active = false;
		g.nodes[i].haslabel = false;
		TAILQ_INSERT_TAIL(&root->nodes, &g.nodes[i], entries);
	}

	for (uintmap::iterator it = g.nodenames.begin(); it != g.nodenames.end(); ++it) {
		g.nodes[it->second].name = it->first;
	}

	for (labelmap::iterator it = g.labelnames.begin(); it != g.labelnames.end(); ++it) {
		label *lab = new label;
		it->second = lab;
		lab->init(it->first);
		TAILQ_INSERT_TAIL(&root->labels, lab, entries);
	}


	printf("%d vertices, %d edges (pos+neg), %d labels, %d label-node pairs\n", cnt, ecnt, uint32_t(g.labelnames.size()), uint32_t(g.links.size()));
}



/************** read edges **********/
void read_edges(graph & g, FILE *f,bool positive, uint32_t &i)
{
	uint32_t a, b;

	while (fscanf(f, "%d%d", &a, &b) == 2) {
		if (a == b) continue;
		edge &E = g.edges[i];
		E.attach(&g.nodes[g.nodenames[a]], &g.nodes[g.nodenames[b]],positive);
		E.is_positive = positive;
		if(!positive)
			g.total_negative_edges++;
		i++;
	}
}

/***************** root_disagreement *******/
double root_disagreement(graph &g){
	return (1.0-g.lambda)*g.total_negative_edges;
}


/************** read labels **********/
void read_labels(graph & g, FILE *f)
{
	uint32_t a, b;
	uint32_t i = 0;

	i = 0;

	while (fscanf(f, "%d%d", &a, &b) == 2) {
		node *u = &g.nodes[g.nodenames[a]];
		label *lab = g.labelnames[b];
		assert(lab);
		labelink *lk = &g.links[i];
		lk->attach(u, lab);
		i++;
	}
}

/************** read **********/
void read(graph & g, FILE *lf, FILE *posf, FILE *negf)
{
	read_sizes(g, lf, posf,negf);
	rewind(lf);
	rewind(posf);
	rewind(negf);
	read_labels(g, lf);
	uint32_t idx = 0;
	read_edges(g, posf, true, idx);
	read_edges(g, negf, false, idx);

	leaf *root = TAILQ_FIRST(&g.leaves);

	label *l;
	TAILQ_FOREACH(l, &root->labels, entries) {
		countedges(l, 0);
	}
	countburden(root, 0, 0);
}

/************** print **********/
void print(graph & g, FILE *out)
{
	leaf *l;
	TAILQ_FOREACH(l, &g.leaves, entries) {

		fprintf(out, "%u ", l->index); 
		
		node *n;
		TAILQ_FOREACH(n, &l->nodes, entries) {
			fprintf(out, "%u ", n->name);
		}
		fprintf(out, "\n");
	}
	
}


void buildtree(graph &g, FILE *out) {
    leaf *root = TAILQ_FIRST(&g.leaves);
    if (!root) return;
	root->index = 0; 
	double root_score = root_disagreement(g);
    fprintf(out, "-1 -1 %f -1 -1\n", root_score);
	 uint32_t index = 1;
    expand(root, g, index, out);
}



/****************** Main ****************/


int main(int argc, char **argv)
{
	static struct option longopts[] = {
		{"pos",  required_argument,  NULL, 'p'},
		{"neg",  required_argument,  NULL, 'n'},
		{"labels", required_argument,  NULL, 'l'},
		{"out",    required_argument,  NULL, 'o'},
		{"tree",   required_argument,  NULL, 't'},
        {"lambda",  required_argument,  NULL, 'b'},
		{"help",   no_argument,        NULL, 'h'},
		{ NULL,    0,                  NULL,  0 }
	};

	char *posname = NULL;
	char *negname = NULL;
	char *labelname = NULL;
	char *outname = NULL;
	char *treename = NULL;
	
    double lambda_val = 0.0;
	bool lambda_set = false;

	int ch;
	while ((ch = getopt_long(argc, argv, "ho:l:p:n:t:b:", longopts, NULL)) != -1) {
	switch (ch) {
		case 'h':
			printf("Usage: %s -p < positive edge file> -n <negative edge file> -l <label file> [-k <cnt>] [-hb] [-t <tree file>] [-o <node file>]\n", argv[0]);
			printf("  -h    print this help\n");
			printf("  -p    positive edge input file\n");
			printf("  -n    negative edge input file\n");
			printf("  -l    label input file\n");
			printf("  -o    node output file\n");
			printf("  -t    tree output file\n");
            printf("  -b    lambda value \n");
			return 0;
			break;
		case 'p':
			posname = optarg;
			break;
		case 'n':
			negname = optarg;
			break;
		case 'l':
			labelname = optarg;
			break;
		case 'o':
			outname = optarg;
			break;
		case 't':
			treename = optarg;
			break;
        case 'b':
            lambda_val= atof(optarg);
        	lambda_set = true;
			break;
		}
	}


	FILE *posf = fopen(posname, "r");
	FILE *negf = fopen(negname, "r");
	FILE *lf = fopen(labelname, "r");

	graph g;
	g.lambda = lambda_val;
	read(g, lf, posf,negf);
	
	fclose(posf);
	fclose(negf);
	fclose(lf);


	FILE *tf = stdout;
	if (treename) tf = fopen(treename, "w");
	buildtree(g, tf);

	FILE *of = stdout;
	if (outname) of = fopen(outname, "w");
	print(g, of);
	
	return 0;
}



