#include <stdio.h>
#include <stdlib.h>
#include <jansson.h>
#include <string.h>
#include <curl/curl.h>
#include <regex.h>
#include <curl/curl.h>

typedef struct auth_data
{
	char *saltapi;
	char *eauth;
	char *username;
	char *password;
} auth_data;

regex_t *regex_match;
int showids;

void print_json_aux(json_t *element, int indent, char *color, char *template, int match);


void print_json_indent(int indent) {
	int i;
	for (i = 0; i < indent; i++) { putchar(' '); }
}

const char *json_plural(int count) {
	return count == 1 ? "" : "s";
}

void print_json_object(json_t *element, int indent, char *template) {
	size_t size;
	const char *key;
	json_t *value;
	if (template)
		strncat(template, ".", 1);

	size = json_object_size(element);

	json_object_foreach(element, key, value)
	{
		char *template2 = 0;
		if (regex_match)
		{
			if (indent > 4)
			{
				template2 = malloc(1000);
				if (template)
					strlcpy(template2, template, 1000);

				uint64_t j;
				strncat(template2, key, strlen(key));
				for (j=0; j<strlen(template2); j++)
					if (template2[j] == '|')
						template2[j] = '_';

				if (showids)
				{
					printf("key %s\n", key);
					printf("%s\n", template2);
				}

				int reti = regexec(regex_match, template2, 0, NULL, 0);
				if (reti)
				{
					// NOT MATCHED
					print_json_aux(value, indent + 2, "\e[1;32m", template2, 0);
				}
				else
				{
					// MATCHED
					if (strcmp(key, "return") && strcmp(key, "pchanges"))
					{
						print_json_indent(indent);
						if (json_typeof(value) == JSON_OBJECT)
							printf("\e[33m%s:\e[0m\n", key);
						else if (json_typeof(value) == JSON_ARRAY)
							printf("\e[33m%s:\e[0m\n", key);
						else
							printf("\e[33m%s:\e[0m ", key);
					}

					print_json_aux(value, indent + 2, "\e[1;32m", template2, 1);
				}
			}
			else
				print_json_aux(value, indent + 2, "\e[1;32m", template2, 0);
		}
		else
		{
			if (strcmp(key, "return") && strcmp(key, "pchanges"))
			{
				print_json_indent(indent);
				if (json_typeof(value) == JSON_OBJECT)
					printf("\e[33m%s:\e[0m\n", key);
				else if (json_typeof(value) == JSON_ARRAY)
					printf("\e[33m%s:\e[0m\n", key);
				else
					printf("\e[33m%s:\e[0m ", key);
			}
			print_json_aux(value, indent + 2, "\e[1;32m", template2, 0);
		}

		if (template2)
			free(template2);
	}
}

void print_json_array(json_t *element, int indent, char* template, int match)
{
	size_t i;
	size_t size = json_array_size(element);
	print_json_indent(indent);

	for (i = 0; i < size; i++) {
		print_json_aux(json_array_get(element, i), indent + 2, NULL, template, match);
	}
}

void print_json_string(json_t *element, char *color)
{
	char *clr = color;
	int mode = 0;
	if (!clr)
	{
	   clr = strdup("\e[1;35m");
	   mode = 1;
	}
	printf("%s%s\e[0m\n", clr, json_string_value(element));

	if (mode)
		free(clr);
}

void print_json_integer(json_t *element, int indent)
{
	print_json_indent(indent);
	printf("\e[1;31m%" JSON_INTEGER_FORMAT "\e[0m\n", json_integer_value(element));
}

void print_json_real(json_t *element, int indent) {
	print_json_indent(indent);
	printf("\e[1;31m%f\e[0m\n", json_real_value(element));
}

void print_json_true(json_t *element, int indent)
{
	(void)element;
	print_json_indent(indent);
	printf("\e[1;35mTrue\e[0m\n");
}

void print_json_false(json_t *element, int indent)
{
	(void)element;
	print_json_indent(indent);
	printf("\e[1;31mFalse\e[0m\n");
}

json_t *load_json(const char *text)
{
	json_t *root;
	json_error_t error;

	root = json_loads(text, 0, &error);

	if (root)
		return root;
	else
	{
		fprintf(stderr, "json error on line %d: %s\n", error.line, error.text);
		return (json_t *)0;
	}
}

void print_json_aux(json_t *element, int indent, char *color, char *template, int match)
{
	switch (json_typeof(element)) {
		case JSON_OBJECT:
			print_json_object(element, indent, template);
			break;
		case JSON_ARRAY:
			print_json_array(element, indent, template, match);
			break;
		case JSON_STRING:
			if (regex_match)
				if (match)
					print_json_string(element, color);
				else {}
			else
				print_json_string(element, color);
			break;
		case JSON_INTEGER:
			if (regex_match)
				if (match)
					print_json_integer(element, indent);
				else {}
			else
				print_json_integer(element, indent);
			break;
		case JSON_REAL:
			if (regex_match)
				if (match)
					print_json_real(element, indent);
				else {}
			else
				print_json_real(element, indent);
			break;
		case JSON_TRUE:
			if (regex_match)
				if (match)
					print_json_true(element, indent);
				else {}
			else
				print_json_true(element, indent);
			break;
		case JSON_FALSE:
			if (regex_match)
				if (match)
					print_json_false(element, indent);
				else {}
			else
				print_json_false(element, indent);
			break;
		default:
			  break;
	}
}


// curl handler
struct string {
	char *ptr;
	size_t len;
};

void init_string(struct string *s) {
	s->len = 0;
	s->ptr = malloc(s->len+1);
	if (s->ptr == NULL) {
		fprintf(stderr, "malloc() failed\n");
		return;
	}
	s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
	size_t new_len = s->len + size*nmemb;
	s->ptr = realloc(s->ptr, new_len+1);
	if (s->ptr == NULL) {
		fprintf(stderr, "realloc() failed\n");
		return 0;
	}
	memcpy(s->ptr+s->len, ptr, size*nmemb);
	s->ptr[new_len] = '\0';
	s->len = new_len;

	return size*nmemb;
}

int64_t curl_handler(char *url, char *body, char **data)
{
	CURL *curl;

	curl = curl_easy_init();
	if(curl) {
		int64_t len;
		struct string s;
		init_string(&s);

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 300000L);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 300000L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
		CURLcode res = curl_easy_perform(curl);
		if ( res != CURLE_OK )
		{
			fprintf(stderr, "api request failed: %s\n", curl_easy_strerror(res));
			curl_easy_cleanup(curl);
			return -1;
		}

		data[0] = s.ptr;
		len = s.len;

		curl_easy_cleanup(curl);
		return len;
	}
	return -1;
}


char *expr_form = 0;
int64_t batch_size = 0;
char *saltenv = 0;
json_t *pillar = 0;
int test = 0;
int dry_run = 0;

char* iterator(int argc, char **argv, int *i)
{
	for (; *i<argc; ++(*i))
	{
		if (!strncmp(argv[*i], "-", 1))
		{
			if (    !strcmp(argv[*i], "--grain")   ||
				!strcmp(argv[*i], "--pillar")  ||
				!strcmp(argv[*i], "--grain-pcre") ||
				!strcmp(argv[*i], "--compound") ||
				!strcmp(argv[*i], "--pillar-pcre") ||
				!strcmp(argv[*i], "--pcre") ||
				!strcmp(argv[*i], "--list"))
				expr_form = strdup(argv[*i]+2);
			else if (!strcmp(argv[*i], "-G"))
				expr_form = strdup("grain");
			else if (!strcmp(argv[*i], "-L"))
				expr_form = strdup("list");
			else if (!strcmp(argv[*i], "-P"))
				expr_form = strdup("grain-pcre");
			else if (!strcmp(argv[*i], "-I"))
				expr_form = strdup("pillar");
			else if (!strcmp(argv[*i], "-C"))
				expr_form = strdup("compound");
			else if (!strcmp(argv[*i], "--dry-run"))
				dry_run = 1;
			else if (!strcmp(argv[*i], "--showids"))
				showids = 1;
			else if (!strcmp(argv[*i], "--match"))
			{
				++(*i);
				regex_match = malloc(sizeof(*regex_match));
				int reti = regcomp(regex_match, argv[*i], REG_EXTENDED);
				if (reti)
				{
					fprintf(stderr, "Could not compile regex: %s\n", argv[*i]);
					exit(1);
				}

			}
			else if (	!strcmp(argv[*i], "--batch-size") ||
					!strcmp(argv[*i], "--batch") ||
					!strcmp(argv[*i], "-b"))
			{
				++(*i);
				batch_size = atoll(argv[*i]);
			}
		}
		else if (strstr(argv[*i], "="))
		{
			if (!strncmp(argv[*i], "saltenv", 7))
				saltenv=strdup(argv[*i]+8);
			else if (!strncmp(argv[*i], "pillar", 6))
				pillar=json_loads(argv[*i]+7, strlen(argv[*i])-7, NULL);
			else if (!strncmp(argv[*i], "test=True", 9)) {}
				test=1;
		}
		else
		{
			return argv[*i];
		}
	}
	return 0;
}

json_t* rsalt_data_load(int argc, char **argv, auth_data *ad)
{
	int i = 1;
	char *tgt = 0;
	char *fun = 0;
	char *arg = 0;
	tgt = iterator(argc, argv, &i);
	i++;
	fun = iterator(argc, argv, &i);
	i++;
	arg = iterator(argc, argv, &i);
	i++;
	iterator(argc, argv, &i);

	json_t *obj = json_object();
	json_t *arg_arr = json_array();
	if (tgt)
		json_object_set_new(obj, "tgt", json_string(tgt));
	if (fun)
		json_object_set_new(obj, "fun", json_string(fun));
	if (arg)
		json_array_append_new(arg_arr, json_string(arg));
	if (saltenv)
		json_array_append_new(arg_arr, json_string(saltenv));
	if (pillar)
		json_object_set(obj, "pillar", pillar);
	if (test)
		json_object_set_new(obj, "test", json_true());
	if (expr_form)
		json_object_set_new(obj, "expr_form", json_string(expr_form));
	if (batch_size)
		json_object_set_new(obj, "batch-size", json_integer(batch_size));
	
	json_object_set_new(obj, "client", json_string("local"));
	json_object_set_new(obj, "arg", arg_arr);
	json_object_set_new(obj, "eauth", json_string(ad->eauth));
	json_object_set_new(obj, "username", json_string(ad->username));
	if (dry_run)
		json_object_set_new(obj, "password", json_string("XXXXXXXXXXXXXXX"));
	else
		json_object_set_new(obj, "password", json_string(ad->password));

	json_t *array = json_array();
	json_array_append(array, obj);
	return array;
}

auth_data* conf_read(char *context, char *filepath)
{
	regex_t regex;
	int reti;

	FILE *fd = fopen(filepath, "r");
	if (!fd)
		return 0;

	reti = regcomp(&regex, "^\[[^]]*", 0);
	if (reti) {
		fprintf(stderr, "Could not compile regex\n");
		exit(1);
	}

	char buf[10000];
	int matched = 0;
	while (fgets(buf, 10000, fd))
	{
		buf[strlen(buf)-1] = 0;
		reti = regexec(&regex, buf, 0, NULL, 0);
		if (!reti)
		{
			if (!strncmp(buf+1, context, strlen(context)))
			{
				matched = 1;
				break;
			}
		}
	}

	if (!matched)
		fprintf(stderr, "context %s not found in file %s\n", context, filepath);

	char *saltapi = 0;
	char *eauth = 0;
	char *username = 0;
	char *password = 0;
	while (fgets(buf, 10000, fd))
	{
		buf[strlen(buf)-1] = 0;
		reti = regexec(&regex, buf, 0, NULL, 0);
		if (!reti)
			break;

		if (!strncmp(buf, "saltapi", strlen("saltapi")))
			saltapi = strdup(buf+8);
		else if (!strncmp(buf, "eauth", strlen("eauth")))
			eauth = strdup(buf+6);
		else if (!strncmp(buf, "username", strlen("username")))
			username = strdup(buf+9);
		else if (!strncmp(buf, "password", strlen("password")))
			password = strdup(buf+9);
	}
	fclose(fd);

	char *tmp;
	tmp = getenv("RSALT_SALTAPI");
	if (tmp)
		saltapi = tmp;
	tmp = getenv("RSALT_EAUTH");
	if (tmp)
		eauth = tmp;
	tmp = getenv("RSALT_USERNAME");
	if (tmp)
		username = tmp;
	tmp = getenv("RSALT_PASSWORD");
	if (tmp)
		password = tmp;

	if (!saltapi)
	{
		fprintf(stderr, "<saltapi> key not found for context %s\n", context);
		return 0;
	}
	else if (!eauth)
	{
		fprintf(stderr, "<eauth> key not found for context %s\n", context);
		return 0;
	}
	if (!username)
	{
		fprintf(stderr, "<username> key not found for context %s\n", context);
		return 0;
	}
	if (!password)
	{
		fprintf(stderr, "<password> key not found for context %s\n", context);
		return 0;
	}

	auth_data *ad = malloc(sizeof(*ad));
	ad->saltapi = saltapi;
	ad->eauth = eauth;
	ad->username = username;
	ad->password = password;

	regfree(&regex);
	return ad;
}

int main(int argc, char **argv)
{
	auth_data *ad = conf_read("default", "/etc/rsalt.conf");
	if (!ad)
		return 2;
	if (argc < 3)
		return 1;

        json_t *obj = rsalt_data_load(argc, argv, ad);
	char *s = json_dumps(obj, 0);
	if (dry_run)
	{
		printf("saltapi: %s\n", ad->saltapi);
		printf("body: %s\n", s);
		return 0;
	}

	char *answ;
	curl_handler(ad->saltapi, s, &answ);
	if (!answ)
	{
		fprintf(stderr, "no answer\n");
		return 4;
	}
	if (strlen(answ)<1)
	{
		fprintf(stderr, "empty answer\n");
		return 3;
	}

	json_t *root = load_json(answ);
	if (root)
	{
		print_json_aux(root, 0, NULL, NULL, 0);
		json_decref(root);
	}
	return 0;
}
