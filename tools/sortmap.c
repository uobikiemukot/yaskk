#include "../common.h"
#include "../util.h"
#include "../list.h"
#include "../hash.h"
#include "../load.h"
#include "../misc.h"
#include "../parse.h"

void swap(struct triplet_t *t1, struct triplet_t *t2)
{
	struct triplet_t tmp;

	tmp = *t1;
	*t1 = *t2;
	*t2 = tmp;
}

void merge(struct triplet_t *tp1, int size1, struct triplet_t *tp2, int size2)
{
	int i, j, count;
	struct triplet_t merged[size1 + size2];

	count = i = j = 0;
	while (i < size1 || j < size2) {
		if (j == size2)
			merged[count++] = tp1[i++];
		else if (i == size1)
			merged[count++] = tp2[j++];
		else if (strcmp(tp1[i].key, tp2[j].key) < 0)
			merged[count++] = tp1[i++];
		else
			merged[count++] = tp2[j++];
	}

	memcpy(tp1, merged, sizeof(struct triplet_t) * (size1 + size2));
}

void merge_sort(struct triplet_t *triplets, int size)
{
	int halfsize;

	if (size <= 1)
		return;
	else if (size == 2) {
		if (strcmp(triplets[0].key, triplets[1].key) > 0)
			swap(&triplets[0], &triplets[1]);
	}
	else {
		halfsize = size / 2;
		merge_sort(triplets, halfsize);
		merge_sort(triplets + halfsize, size - halfsize);
		merge(triplets, halfsize, triplets + halfsize, size - halfsize);
	}
}

int main(int argc, char *argv[])
{
	int i;
	const char *file;
	struct map_t map;

	if (argc < 2) {
		fprintf(stderr, "usage: ./sortmap MAP\n");
		exit(EXIT_FAILURE);
	}
	file = argv[1];

	map.triplets = NULL;
	map.count = 0;
	load_map(file, &map);

	merge_sort(map.triplets, map.count);

	for (i = 0; i < map.count; i++)
		printf("%s\t%s\t%s\n", map.triplets[i].key, map.triplets[i].hira, map.triplets[i].kata);
}
