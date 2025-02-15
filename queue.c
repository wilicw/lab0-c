#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */

static inline element_t *e_new(char *s);
static inline int q_de_a_scend(struct list_head *head, bool descend);

/* Create an empty queue */
struct list_head *q_new()
{
    struct list_head *list = malloc(sizeof(struct list_head));
    if (list)
        INIT_LIST_HEAD(list);
    return list;
}

/* Free all storage used by queue */
void q_free(struct list_head *l)
{
    if (!l)
        return;

    element_t *element, *safe;
    list_for_each_entry_safe (element, safe, l, list)
        q_release_element(element);
    free(l);
}

/* Insert an element at head of queue */
bool q_insert_head(struct list_head *head, char *s)
{
    if (!head)
        return false;

    element_t *element = e_new(s);
    if (!element)
        return false;
    list_add(&element->list, head);
    return true;
}

/* Insert an element at tail of queue */
bool q_insert_tail(struct list_head *head, char *s)
{
    if (!head)
        return false;

    element_t *element = e_new(s);
    if (!element)
        return false;
    list_add_tail(&element->list, head);
    return true;
}

/* Remove an element from head of queue */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;

    struct list_head *node = head->next;
    element_t *element = list_entry(node, element_t, list);
    if (sp && element->value) {
        memcpy(sp, element->value, bufsize);
        sp[bufsize - 1] = 0;
    }
    list_del(node);
    return element;
}

/* Remove an element from tail of queue */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;

    struct list_head *node = head->prev;
    element_t *element = list_entry(node, element_t, list);
    if (sp && element->value) {
        memcpy(sp, element->value, bufsize);
        sp[bufsize - 1] = 0;
    }
    list_del(node);
    return element;
}

/* Return number of elements in queue */
int q_size(struct list_head *head)
{
    int n = 0;
    struct list_head *node;
    list_for_each (node, head)
        ++n;
    return n;
}

/* Delete the middle node in queue */
bool q_delete_mid(struct list_head *head)
{
    // https://leetcode.com/problems/delete-the-middle-node-of-a-linked-list/
    if (!head || list_empty(head))
        return false;

    struct list_head *fast, *slow, *mid;
    slow = head;
    for (fast = (head)->next; fast != head && fast->next != head;
         fast = fast->next->next)
        slow = slow->next;
    mid = slow->next;
    list_del(mid);
    element_t *element = list_entry(mid, element_t, list);
    q_release_element(element);
    return true;
}

/* Delete all nodes that have duplicate string */
bool q_delete_dup(struct list_head *head)
{
    // https://leetcode.com/problems/remove-duplicates-from-sorted-list-ii/
    if (!head || list_empty(head))
        return false;

    struct list_head *node, *safe;
    bool mark_del = false;
    list_for_each_safe (node, safe, head) {
        element_t *cur = list_entry(node, element_t, list);
        element_t *nxt = list_entry(node->next, element_t, list);
        if (node->next != head && strcmp(cur->value, nxt->value) == 0) {
            list_del(node);
            q_release_element(cur);
            mark_del = true;
        } else if (mark_del) {
            list_del(node);
            q_release_element(cur);
            mark_del = false;
        }
    }
    return true;
}

/* Swap every two adjacent nodes */
void q_swap(struct list_head *head)
{
    // https://leetcode.com/problems/swap-nodes-in-pairs/
    if (!head || list_empty(head))
        return;

    struct list_head *prev = head;
    while (prev->next != head && prev->next->next != head) {
        struct list_head *node = prev->next;
        list_move_tail(node, node->next->next);
        prev = node;
    }
}

/* Reverse elements in queue */
void q_reverse(struct list_head *head)
{
    if (!head || list_empty(head))
        return;

    struct list_head *current = head;
    do {
        struct list_head *next = current->next;
        current->next = current->prev;
        current->prev = next;
        current = next;
    } while (current != head);
}

/* Reverse the nodes of the list k at a time */
void q_reverseK(struct list_head *head, int k)
{
    // https://leetcode.com/problems/reverse-nodes-in-k-group/
    if (!head || list_empty(head))
        return;

    LIST_HEAD(new_head);
    struct list_head *cur = head->next;

    while (cur != head) {
        struct list_head *next;
        struct list_head *old_cur = cur;
        int cnt = 0;
        bool reverse;

        while (cnt < k - 1 && cur->next != head) {
            cur = cur->next;
            ++cnt;
        }
        next = cur->next;
        cnt += 1;

        reverse = cnt == k;
        cur = reverse ? cur : old_cur;
        while (cnt--) {
            struct list_head *node = reverse ? cur->prev : cur->next;
            list_del(cur);
            list_add_tail(cur, &new_head);
            cur = node;
        }

        cur = next;
    }

    list_splice(&new_head, head);
}

struct list_head *q_sort_merge(struct list_head *head, bool descend)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return head;

    struct list_head *fast, *slow, *mid;
    slow = head;
    for (fast = (head)->next; fast != head && fast->next != head;
         fast = fast->next->next)
        slow = slow->next;
    mid = slow;

    LIST_HEAD(left);
    struct list_head *right = head;
    list_cut_position(&left, head, mid);

    struct list_head *left_sorted = q_sort_merge(&left, descend);
    struct list_head *right_sorted = q_sort_merge(right, descend);

    LIST_HEAD(sorted);
    queue_contex_t left_ctx = {.q = left_sorted};
    queue_contex_t right_ctx = {.q = right_sorted};
    list_add_tail(&left_ctx.chain, &sorted);
    list_add_tail(&right_ctx.chain, &sorted);
    q_merge(&sorted, descend);
    list_splice(list_entry(sorted.next, queue_contex_t, chain)->q, head);
    return head;
}

/* Sort elements of queue in ascending/descending order */
void q_sort(struct list_head *head, bool descend)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;

    q_sort_merge(head, descend);
}

/* Remove every node which has a node with a strictly less value anywhere to
 * the right side of it */
int q_ascend(struct list_head *head)
{
    // https://leetcode.com/problems/remove-nodes-from-linked-list/
    return q_de_a_scend(head, false);
}

/* Remove every node which has a node with a strictly greater value anywhere to
 * the right side of it */
int q_descend(struct list_head *head)
{
    // https://leetcode.com/problems/remove-nodes-from-linked-list/
    return q_de_a_scend(head, true);
}

/* Merge all the queues into one sorted queue, which is in ascending/descending
 * order */
int q_merge(struct list_head *head, bool descend)
{
    // https://leetcode.com/problems/merge-k-sorted-lists/
    if (!head || list_empty(head))
        return 0;

    int cnt = 0;

    LIST_HEAD(sorted);
    struct list_head *node;

    for (;;) {
        struct list_head *target = NULL;
        list_for_each (node, head) {
            queue_contex_t *ctx = list_entry(node, queue_contex_t, chain);
            if (list_empty(ctx->q))
                continue;

            struct list_head *cmp_node = ctx->q->next;
            if (!target) {
                target = cmp_node;
            } else {
                int cmp_result =
                    strcmp(list_entry(cmp_node, element_t, list)->value,
                           list_entry(target, element_t, list)->value);
                if (descend ? (cmp_result >= 0) : (cmp_result < 0))
                    target = cmp_node;
            }
        }

        if (!target)
            break;

        list_del(target);
        list_add_tail(target, &sorted);
        ++cnt;
    }

    list_splice(&sorted, list_entry(head->next, queue_contex_t, chain)->q);

    return cnt;
}

static inline element_t *e_new(char *s)
{
    element_t *element = malloc(sizeof(element_t));
    char *es = strdup(s);
    if (!element || !es) {
        free(element);
        free(es);
        return NULL;
    }
    element->value = es;
    return element;
}

static inline int q_de_a_scend(struct list_head *head, bool descend)
{
    if (!head || list_empty(head))
        return 0;

    int cnt = 0;
    struct list_head *cur = descend ? head->prev : head->next;
    struct list_head *safe;
    char *cmp_str = NULL;
    while (cur != head) {
        safe = descend ? cur->prev : cur->next;
        element_t *element = list_entry(cur, element_t, list);
        if (cmp_str && strcmp(cmp_str, element->value) >= 0) {
            list_del(cur);
            q_release_element(element);
        } else {
            cmp_str = element->value;
            ++cnt;
        }
        cur = safe;
    }
    return cnt;
}
