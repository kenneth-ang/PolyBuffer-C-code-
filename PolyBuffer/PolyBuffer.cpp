/*
Polygon buffer Program




Author: Kenneth Ang
*/

#include "stdafx.h"
#include "stdio.h"
#include "stdlib.h"
#include "SLL.h"
#include "geometry.h"
#include "PQ.h"
#include "limits"

//Number of vertices of initial polygon
#define NUMPTS 5

//Global variable should only be accessed using assign_id() function 
int uniqueid = 0;
/*Used to assign a unique id to each ertex created in the linked list*/
int assign_id();

/*Uses pointers to x and y values corresponding to vertices on a given polygon
Note: Vertices should be aligned counterclockwise
*/
sll_t *initialize_slav(double *x, double *y, int numpts, double dist);

/*
Initializes the edges between adjacent vertices
This function initialized the edges for every vertex in the lav
*/
void initialize_edges(ll_t *lav); 
/*
Initializes the angular bisector of each vertex in the lav
*/
void initialize_bisectors(ll_t *lav); 

/*
Main function containing the algorithm to buffer original polygon
This function returns the buffered polygon
*/
sll_t *buffer_polygon(sll_t *slav);

/*Function buffers input lav by dist*/
ll_t *restructure_poly(ll_t *lav, double dist);
/*Function moves p1 along its angular bisector by dist distance
p2 is the next point in the lav and is used to ensure p1 s moved in the right direction
dir should be 1 to move p1 inwards and -1 to move p1 outwards*/
point_t *offsetpt(point_t *p1, point_t *p2, double dist, double dir);

/*Obtain the next closest event of the current lav*/
node_t *get_poly_event(ll_t *lav); 
/*Obtain a priority queue of all the events of an input lav*/
pq_t *build_PQ(ll_t *lav); 
/*Obtain closest event of a vertex*/
node_t *vert_event(vert_t *currvert, ll_t *lav); 
/*Finds the closest edge event (If it exists) of a vertex*/
node_t *edge_point(vert_t *convexvert); 
/*Finds the closest split event of a reflex vertex*/
node_t *split_point(vert_t *reflexvert, ll_t *lav);

/*Handles event*/
ll_t *handle_event(ll_t *lav, node_t *polyevent); 
ll_t *handle_edge_event(ll_t *lav, node_t *edgeevent);
ll_t *handle_split_event(ll_t *lav, node_t *splitevent);

/*Check of possible split event is within bounds*/
bool in_bounds(point_t *bisectpt1, point_t *pt, point_t *bisectpt2, point_t *vertpt);


int main()
{
	double bufferdist = 0.5; 
	double *xcoords = (double*)malloc(sizeof(*xcoords)*NUMPTS);
	double *ycoords = (double*)malloc(sizeof(*ycoords)*NUMPTS);

	//Assigning of xcoords, ycoords and bufferdist depends on the system 
	//and how the programmer wishes to impliment it
	double x[NUMPTS] = {-2,-2,2,2,0};
	double y[NUMPTS] = {2,-2,-2,2,4};
	int i;
	for (i = 0; i < NUMPTS; i++)
	{
		xcoords[i] = x[i];
		ycoords[i] = y[i];
	}


	//Initializes original polygon
	sll_t *unprocessed_slav = initialize_slav(x,y,NUMPTS,bufferdist); 
	initialize_edges(unprocessed_slav->currentlst);
	initialize_bisectors(unprocessed_slav->currentlst);
	printf("Original Coordinates\n");
	printSLL(unprocessed_slav);

	//Obtain buffered polygon
	sll_t *polygon = buffer_polygon(unprocessed_slav);
	printf("\n");
	printf("Buffer Distance = %f.\n\nBuffered Coordinates:\n", bufferdist);
	printSLL(polygon);
	
    return 0;
}



//Functions
int assign_id()
{
	int id = uniqueid;
	uniqueid++;
	return id;
}

sll_t * initialize_slav(double *x, double *y, int numpts, double dist)
{
	sll_t *slav = sll_init(); 
	ll_t *lav = ll_init(dist); 
	int count;
	for (count = 0; count < NUMPTS; count++)
	{
		point_t *newpt = init_point(x[count], y[count]);
		vert_t *newvert = vert_init(assign_id(), newpt);
		insertLLcurrent(lav, newvert); 
	}
	lav->remdist = dist; 
	insertSLL(slav, lav); 
	return slav; 
}

void initialize_edges(ll_t *lav)
{
	int count; 
	for (count = 0; count < lav->numvert; count++)
	{
		point_t *curr = lav->current->data;
		point_t *prev = lav->current->prev->data;
		point_t *nxt = lav->current->next->data;
		curr->edgei_1 = create_line_seg(prev, curr);
		curr->edgei = create_line_seg(curr, nxt);
		lav->current->data = curr;
		incrementLLcurrent(lav);
	}
}

void initialize_bisectors(ll_t *lav)
{
	int count; 
	for (count = 0; count < NUMPTS; count++)
	{
		vert_t *currvert = lav->current;
		assign_bisector(currvert->prev->data, currvert->data, currvert->next->data);
		incrementLLcurrent(lav); 
	}
}

node_t *get_poly_event(ll_t *lav)
{
	pq_t *distQ = build_PQ(lav);
	return pq_pop(distQ); 
}

pq_t *build_PQ(ll_t *lav)
{
	pq_t *newQ = pq_init();
	vert_t *currvert = lav->current;
	int i;
	for (i = 0; i < lav->numvert; i++)
	{
		node_t *nxtevent = vert_event(currvert,lav);
		if (nxtevent == NULL)
		{
			currvert = currvert->next;
			continue;
		}
		newQ->root = pq_insert(newQ->root, nxtevent);
		currvert = currvert->next;
	}
	return newQ; 
}

node_t *vert_event(vert_t *currvert, ll_t *lav)
{
	
	node_t *edgepoint = edge_point(currvert);
	node_t *splitpoint = NULL;
	if (left_of_ray(currvert->data, currvert->prev->data, currvert->next->data))
	{
		splitpoint = split_point(currvert, lav);
	}

	if (splitpoint == NULL &&edgepoint == NULL)
	{
		return NULL; 
	}

	if (splitpoint == NULL)
	{
		return edgepoint;
	}
	else if (edgepoint == NULL)
	{
		return splitpoint;
	}
	else if (splitpoint->priority > edgepoint->priority)
	{
		return edgepoint;
	}
	else
	{
		return splitpoint;
	}
}

node_t *edge_point(vert_t *convexvert)
{
	vert_t *prevvert = convexvert->prev;
	vert_t *nxtvert = convexvert->next;
	point_t *interi_1 = edge_intersection(prevvert->data, convexvert->data);
	point_t *interi = edge_intersection(convexvert->data, nxtvert->data);
	double disti_1 = INFINITY;
	double disti = INFINITY;
	if (interi_1 != NULL) disti_1 = dist_to_line(interi_1, prevvert->data);
	if (interi != NULL) disti = dist_to_line(interi, convexvert->data);

	node_t *newnode = NULL;

	if (disti == INFINITY && disti_1 == INFINITY) return newnode;

	if (disti >= disti_1)
	{
		newnode = node_init(disti_1, interi_1, prevvert->id, convexvert->id, EDGE);
		assign_bisector(prevvert->data, newnode->intersection, convexvert->data);
	}
	else if (disti < disti_1)
	{
		newnode = node_init(disti, interi, convexvert->id, nxtvert->id, EDGE);
		assign_bisector(convexvert->data, newnode->intersection, nxtvert->data);
	}
	return newnode;
}

node_t *split_point(vert_t *reflexvert, ll_t *lav)
{
	point_t *concpt = reflexvert->data, *prvpt = reflexvert->prev->data, *nxtpt = reflexvert->next->data;
	double shortest = INFINITY;
	double disttoedge;
	vert_t *currvert = lav->current;
	point_t *curr_split = NULL;
	node_t *splitintersect = NULL;
	int i, startedgeid;

	for (i = 0; i < lav->numvert; i++)
	{
		if (currvert->id == reflexvert->id || currvert->id == reflexvert->prev->id)
		{
			currvert = currvert->next;
			continue;
		}
		point_t *splitpt = split_intersection(prvpt, concpt, nxtpt, currvert->data);
		if (splitpt == NULL)
		{
			currvert = currvert->next;
			continue;
		}
		double dist = dist_points(concpt, splitpt);
		
		
		if (in_bounds(currvert->data, splitpt, currvert->next->data,concpt) && dist<shortest)
		{

			curr_split = splitpt;
			disttoedge = dist_to_line(splitpt, reflexvert->data);
			shortest = dist;
			startedgeid = currvert->id;

		}
		currvert = currvert->next;
	}
	if (curr_split != NULL)
	{
		//Note: parAid is the concave vertex id and 
		//parBid the id of the vertex at start of the edge the concave vertex splits
		splitintersect = node_init(disttoedge, curr_split, reflexvert->id, startedgeid,SPLIT);
	}
	return splitintersect;

}

bool in_bounds(point_t *bisectpt1, point_t *pt, point_t *bisectpt2, point_t *vertpt)
{
	bool bounded = false; 
	line_t *B1 = bisectpt1->bisector, *B2 = bisectpt2->bisector; 
	point_t *p = NULL; 
	if (B1->grad != B2->grad)
	{
		p = intersection_point(B1, bisectpt1->xcoord, B2, bisectpt2->xcoord);
		if (left_of_ray(p, bisectpt1, bisectpt2) == false)
		{
			p = NULL; 
		}
	}
	if (p != NULL)
	{
		if (left_of_ray(pt, bisectpt1, p) == false && left_of_ray(pt, bisectpt2, p) == true) bounded = true; 
	}
	else
	{
		line_t *edge = bisectpt1->edgei; 
		line_t *parallel_edge; 
		double x; 
		double y_inter1 = NULL; 
		if (edge->grad == INFINITY || edge->grad == -INFINITY)
		{
			x = vertpt->xcoord; 
		}
		else
		{
            y_inter1 = vertpt->ycoord - edge->grad * vertpt->xcoord; 
		}
		parallel_edge = init_line(edge->grad, y_inter1);
		point_t *E1 = intersection_point(B1, bisectpt1->xcoord, parallel_edge,vertpt->xcoord);
		point_t *E2 = intersection_point(B2, bisectpt2->xcoord, parallel_edge,vertpt->xcoord);
		if (E1 == NULL || E2 == NULL)
		{
			return false; 
		}
		if (left_of_ray(pt, bisectpt1, E1) == false && left_of_ray(pt, bisectpt2, E2) == true) bounded = true; 
	}
	return bounded; 
}

sll_t *buffer_polygon(sll_t *slav)
{
	sll_t *buffered_poly = sll_init(); 

	while (slav->currentlst != NULL)
	{
		
		ll_t *lav = removeSLL(slav);
		ll_t *handled_lav; 
		
		if (lav->remdist == 0)
		{
			//printf("1\n");
			insertSLL(buffered_poly, lav);
			continue; 
		}
		else if (lav->remdist < 0)
		{
			//printf("12\n");
			continue; 
		}
		//printf("3\n");
		node_t *polyevent = get_poly_event(lav); 
		if (polyevent == NULL)
		{
			
			//printf("Null Event\n");
			continue; 
		}else
		{ 
			//printf("gotevent\n");
		}
		handled_lav = handle_event(lav, polyevent); 
		//printf("Handled event\n");
		insertSLL(slav, handled_lav);
		//printSLL(slav);
		//printf("CURRENT SLAV");
		//printSLL(slav);
	}
	return buffered_poly; 
}

ll_t *handle_event(ll_t *lav, node_t *polyevent)
{
	ll_t *newlav;
	double dist = lav->remdist;
	if (polyevent->priority < lav->remdist)
	{
		if (polyevent->type == EDGE)
		{
			//Handle Edge event
			newlav = handle_edge_event(lav, polyevent);
			newlav = restructure_poly(lav, polyevent->priority); 
		}
		else
		{
			//Handle split event
			//Do not restructure when handling split
			newlav = handle_split_event(lav, polyevent);
			initialize_edges(lav);
			initialize_bisectors(lav);
			initialize_edges(lav->nextlst);
			initialize_bisectors(lav->nextlst);
		}
		//set dist to polyevent distance
		dist = polyevent->priority;
	}
	else
	{
		//No events are hit, can just restructure
		newlav = restructure_poly(lav, lav->remdist);
	}
	return newlav;
}

ll_t *handle_edge_event(ll_t *lav, node_t *edgeevent)
{
	//Obtain edge's adjacent vertex points
	changeLLcurrent(lav, edgeevent->Aid); 
	point_t *parA = lav->current->data;
	point_t *parB = lav->current->next->data; 
	//Intersection point of the edge event's adjacent edges
	point_t *newpt = intersection_point(parA->edgei_1,parA->xcoord, parB->edgei, parB->xcoord); 
	newpt->edgei_1 = parA->edgei_1; 
	newpt->edgei = parB->edgei; 
	vert_t *newvert = vert_init(assign_id(), newpt); 
	//remove edge's adjacent vertex points from lav
	vert_t *A = removeLLcurrent(lav);
	vert_t *B = removeLLcurrent(lav);
	lav->current = lav->current->prev;
	//insert new intersection vertex
	insertLLcurrent(lav, newvert);
	return lav; 
}

ll_t *handle_split_event(ll_t *lav, node_t *splitevent)
{
	changeLLcurrent(lav, splitevent->Aid);
	vert_t *v = lav->current;

	changeLLcurrent(lav, splitevent->Bid);
	vert_t *se = lav->current;

	line_t *prvE = v->data->edgei_1, *nxtE = v->data->edgei;
	line_t *edge = se->data->edgei;

	point_t *p1 = intersection_point(prvE, v->data->xcoord, edge, se->data->xcoord);
	point_t *p2 = intersection_point(nxtE, v->data->xcoord, edge, se->data->xcoord);

	p1->edgei_1 = v->data->edgei_1;
	p1->edgei = se->data->edgei; 
	p2->edgei_1 = se->data->edgei; 
	p2->edgei = v->data->edgei; 
	vert_t *v1 = vert_init(assign_id(), p1);
	vert_t *v2 = vert_init(assign_id(), p2);
	changeLLcurrent(lav, splitevent->Aid); 
	lav = split_reflex(lav, v1, v2, splitevent->Bid);
	return lav; 
}

ll_t *restructure_poly(ll_t * lav, double dist)
{
	initialize_edges(lav);
	initialize_bisectors(lav);
	vert_t *curr = lav->current;
	ll_t *newlav = ll_init(lav->remdist -= dist);
	int i; 
	for (i = 0; i < lav->numvert; i++)
	{
		point_t *shiftpt = offsetpt(curr->data, curr->next->data, dist, 1);
		shiftpt->edgei_1 = curr->data->edgei_1; 
		shiftpt->edgei = curr->data->edgei; 
		shiftpt->bisector = curr->data->bisector; 
		insertLLcurrent(newlav, vert_init(assign_id(), shiftpt));
		curr = curr->next; 
	}
	initialize_edges(newlav);
	initialize_bisectors(newlav);
	return newlav;
}

point_t *offsetpt(point_t *p1, point_t *p2, double dist, double dir)
{
	double xnorm = -1*(p2->ycoord - p1->ycoord);
	double ynorm = p2->xcoord - p1->xcoord; 
	double mag = sqrt(pow(xnorm,2)+pow(ynorm,2));
	xnorm /= mag; 
	ynorm /= mag; 
	double tempx = p1->xcoord + dir*dist*xnorm; 
	double ytemp = p1->ycoord + dir*dist*ynorm; 
	double t_y_intersect; 

	if (p1->edgei->grad == INFINITY || p1->edgei->grad == -INFINITY)
	{
		t_y_intersect = INFINITY;
	}
	else
	{
		t_y_intersect = ytemp - (tempx*p1->edgei->grad);
	}

	line_t *tempL = init_line(p1->edgei->grad, t_y_intersect); 
	point_t *offpt = intersection_point(p1->bisector,p1->xcoord,tempL,tempx); 
	return offpt; 
}

