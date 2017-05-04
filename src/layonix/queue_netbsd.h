/*      $NetBSD: queue.h,v 1.52 2009/04/20 09:56:08 mschuett Exp $ */

/*
 * Qemu version: Copy from netbsd, removed debug code, removed some of
 * the implementations.  Left in lists, simple queues, tail queues and
 * circular queues.
 */

/*
 * Copyright (c) 1991, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)queue.h     8.5 (Berkeley) 8/20/94
 */

#ifndef QUEUE_H_
#define QUEUE_H_
#define NULL (void*)0

#define Q_HEAD(name, type)                                          \
struct name {                                                       \
        struct type *first;                                         \
        struct type **last;                                         \
}

#define Q_ENTRY(type)                                               \
struct {                                                            \
        struct type *next;                                          \
        struct type **prev;                                         \
}

#define Q_INIT(head) do {                                           \
        (head)->first = NULL;                                       \
        (head)->last = &(head)->first;                              \
} while (0)

#define Q_INSERT_HEAD(head, elm, field) do {                        \
        if (((elm)->field.next = (head)->first) != NULL)            \
                (head)->first->field.prev =                         \
                    &(elm)->field.next;                             \
        else                                                        \
                (head)->last = &(elm)->field.next;                  \
        (head)->first = (elm);                                      \
        (elm)->field.prev = &(head)->first;                         \
} while (0)

#define Q_INSERT_TAIL(head, elm, field) do {                        \
        (elm)->field.next = NULL;                                   \
        (elm)->field.prev = (head)->last;                           \
        *(head)->last = (elm);                                      \
        (head)->last = &(elm)->field.next;                          \
} while (0)

#define Q_INSERT_AFTER(head, listelm, elm, field) do {              \
        if (((elm)->field.next = (listelm)->field.next) != NULL)    \
                (elm)->field.next->field.prev =                     \
                    &(elm)->field.next;                             \
        else                                                        \
                (head)->last = &(elm)->field.next;                  \
        (listelm)->field.next = (elm);                              \
        (elm)->field.prev = &(listelm)->field.next;                 \
} while (0)

#define Q_INSERT_BEFORE(listelm, elm, field) do {                   \
        (elm)->field.prev = (listelm)->field.prev;                  \
        (elm)->field.next = (listelm);                              \
        *(listelm)->field.prev = (elm);                             \
        (listelm)->field.prev = &(elm)->field.next;                 \
} while (0)

#define Q_REMOVE(head, elm, field) do {                             \
        if (((elm)->field.next) != NULL)                            \
                (elm)->field.next->field.prev =                     \
                    (elm)->field.prev;                              \
        else                                                        \
                (head)->last = (elm)->field.prev;                   \
        *(elm)->field.prev = (elm)->field.next;                     \
} while (0)

#define Q_FOREACH(var, head, field)                                 \
        for ((var) = ((head)->first); (var); (var) = ((var)->field.next))

#define Q_EMPTY(head)               ((head)->first == NULL)
#define Q_FIRST(head)               ((head)->first)
#define Q_NEXT(elm, field)          ((elm)->field.next)

#define Q_LAST(head, headname)                                      \
        (*(((struct headname *)((head)->last))->last))
#define Q_PREV(elm, headname, field)                                \
        (*(((struct headname *)((elm)->field.prev))->last))

#endif
