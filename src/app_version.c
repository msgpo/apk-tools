/* app_version.c - Alpine Package Keeper (APK)
 *
 * Copyright (C) 2005-2008 Natanael Copa <n@tanael.org>
 * Copyright (C) 2008-2011 Timo Teräs <timo.teras@iki.fi>
 * All rights reserved.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <stdio.h>
#include "apk_defines.h"
#include "apk_applet.h"
#include "apk_database.h"
#include "apk_version.h"
#include "apk_print.h"

struct ver_ctx {
	int (*action)(struct apk_database *db, struct apk_string_array *args);
	const char *limchars;
	int all_tags : 1;
};

static int ver_indexes(struct apk_database *db, struct apk_string_array *args)
{
	struct apk_repository *repo;
	int i;

	for (i = 0; i < db->num_repos; i++) {
		repo = &db->repos[i];

		if (APK_BLOB_IS_NULL(repo->description))
			continue;

		printf(BLOB_FMT " [%s]\n",
		       BLOB_PRINTF(repo->description),
		       db->repos[i].url);
	}

	return 0;
}

static int ver_test(struct apk_database *db, struct apk_string_array *args)
{
	int r;

	if (args->num != 2)
		return 1;

	r = apk_version_compare(args->item[0], args->item[1]);
	printf("%s\n", apk_version_op_string(r));
	return 0;
}

static int ver_validate(struct apk_database *db, struct apk_string_array *args)
{
	char **parg;
	int errors = 0;

	foreach_array_item(parg, args) {
		if (!apk_version_validate(APK_BLOB_STR(*parg))) {
			if (apk_verbosity > 0)
				printf("%s\n", *parg);
			errors++;
		}
	}
	return errors;
}

enum {
	OPT_VERSION_all,
	OPT_VERSION_check,
	OPT_VERSION_indexes,
	OPT_VERSION_limit,
	OPT_VERSION_test,
};

static const char option_desc[] =
	APK_OPTAPPLET
	APK_OPT2n("all", "a")
	APK_OPT2n("check", "c")
	APK_OPT2n("indexes", "I")
	APK_OPT2R("limit", "l")
	APK_OPT2n("test", "t");

static int option_parse_applet(void *ctx, struct apk_db_options *dbopts, int opt, const char *optarg)
{
	struct ver_ctx *ictx = (struct ver_ctx *) ctx;
	switch (opt) {
	case OPT_VERSION_all:
		ictx->all_tags = 1;
		break;
	case OPT_VERSION_check:
		ictx->action = ver_validate;
		dbopts->open_flags |= APK_OPENF_NO_STATE | APK_OPENF_NO_REPOS;
		break;
	case OPT_VERSION_indexes:
		ictx->action = ver_indexes;
		break;
	case OPT_VERSION_limit:
		ictx->limchars = optarg;
		break;
	case OPT_VERSION_test:
		ictx->action = ver_test;
		dbopts->open_flags |= APK_OPENF_NO_STATE | APK_OPENF_NO_REPOS;
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

static const struct apk_option_group optgroup_applet = {
	.desc = option_desc,
	.parse = option_parse_applet,
};

static void ver_print_package_status(struct apk_database *db, const char *match, struct apk_name *name, void *pctx)
{
	struct ver_ctx *ctx = (struct ver_ctx *) pctx;
	struct apk_package *pkg;
	struct apk_provider *p0;
	char pkgname[41];
	const char *opstr;
	apk_blob_t *latest = apk_atomize(&db->atoms, APK_BLOB_STR(""));
	unsigned int latest_repos = 0;
	int i, r = -1;
	unsigned short tag, allowed_repos;

	if (!name) return;

	pkg = apk_pkg_get_installed(name);
	if (!pkg) return;

	tag = pkg->ipkg->repository_tag;
	allowed_repos = db->repo_tags[tag].allowed_repos;

	foreach_array_item(p0, name->providers) {
		struct apk_package *pkg0 = p0->pkg;
		if (pkg0->name != name || pkg0->repos == 0)
			continue;
		if (!(ctx->all_tags || (pkg0->repos & allowed_repos)))
			continue;
		r = apk_version_compare_blob(*pkg0->version, *latest);
		switch (r) {
		case APK_VERSION_GREATER:
			latest = pkg0->version;
			latest_repos = pkg0->repos;
			break;
		case APK_VERSION_EQUAL:
			latest_repos |= pkg0->repos;
			break;
		}
	}
	r = latest->len ? apk_version_compare_blob(*pkg->version, *latest)
			: APK_VERSION_UNKNOWN;
	opstr = apk_version_op_string(r);
	if ((ctx->limchars != NULL) && (strchr(ctx->limchars, *opstr) == NULL))
		return;
	if (apk_verbosity <= 0) {
		printf("%s\n", pkg->name->name);
		return;
	}

	tag = APK_DEFAULT_REPOSITORY_TAG;
	for (i = 1; i < db->num_repo_tags; i++) {
		if (latest_repos & db->repo_tags[i].allowed_repos) {
			tag = i;
			break;
		}
	}

	snprintf(pkgname, sizeof(pkgname), PKG_VER_FMT, PKG_VER_PRINTF(pkg));
	printf("%-40s%s " BLOB_FMT " " BLOB_FMT "\n",
		pkgname, opstr,
		BLOB_PRINTF(*latest),
		BLOB_PRINTF(db->repo_tags[tag].tag));
}

static int ver_main(void *pctx, struct apk_database *db, struct apk_string_array *args)
{
	struct ver_ctx *ctx = (struct ver_ctx *) pctx;

	if (ctx->limchars) {
		if (strlen(ctx->limchars) == 0)
			ctx->limchars = NULL;
	} else if (args->num == 0 && apk_verbosity == 1) {
		ctx->limchars = "<";
	}

	if (ctx->action != NULL)
		return ctx->action(db, args);

	if (apk_verbosity > 0)
		printf("%-42sAvailable:\n", "Installed:");

	apk_name_foreach_matching(
		db, args, APK_FOREACH_NULL_MATCHES_ALL | apk_foreach_genid(),
		ver_print_package_status, ctx);

	return 0;
}

static struct apk_applet apk_ver = {
	.name = "version",
	.open_flags = APK_OPENF_READ,
	.context_size = sizeof(struct ver_ctx),
	.optgroups = { &optgroup_global, &optgroup_applet },
	.main = ver_main,
};

APK_DEFINE_APPLET(apk_ver);
