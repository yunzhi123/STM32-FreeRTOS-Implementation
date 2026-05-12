#include "list.h"

struct xLIST List_Test;

struct xLIST_ITEM List_item1;
struct xLIST_ITEM List_item2;
struct xLIST_ITEM List_item3;

int main() {
	vListInitialise(&List_Test);
	
	vListInitialiseItem(&List_item1);
	listSET_LIST_ITEM_VALUE(&List_item1, 1);
	
	vListInitialiseItem(&List_item2);
	listSET_LIST_ITEM_VALUE(&List_item2, 2);
	
	vListInitialiseItem(&List_item3);
	listSET_LIST_ITEM_VALUE(&List_item3, 3);
	
	vListInsert(&List_Test, &List_item2);
	vListInsert(&List_Test, &List_item1);
	vListInsert(&List_Test, &List_item3);
	while (1);
}
