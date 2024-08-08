# Script for cleaning old versions of packages in GitHub Packages
# Author: https://github.com/ctapmex

from ghapi.all import GhApi
from ghapi.page import paged
import os
import re


def get_version_for_remove(package_versions):
    sorted_version = sorted(package_versions, key=lambda d: d['created_at'], reverse=True)
    # remove from list actual version
    del sorted_version[0]
    return sorted_version


org_name = os.getenv('ORG_NAME', None)
package_mask = os.getenv('PACKAGE_MASK', None)
package_type = os.getenv('PACKAGE_TYPE', None)
github_token = os.getenv('GITHUB_TOKEN', None)

if not org_name or not package_type or not package_mask or not github_token:
    raise BaseException("not all significant parameters are set")

api = GhApi(token=github_token)

package_pages = paged(api.packages.list_packages_for_organization, org=org_name,
                      package_type=package_type, visibility=None, per_page=100)

print("search packages")
for package_page in package_pages:
    for package in package_page:
        if re.search(package_mask, package['name']):
            print("found package - ", package['name'])
            package_version = []
            version_pages = paged(api.packages.get_all_package_versions_for_package_owned_by_org, org=org_name,
                                  package_name=package['name'], package_type=package['package_type'], state='active',
                                  per_page=100)
            for version_page in version_pages:
                for version in version_page:
                    vp = {'version_id': version['id'], 'created_at': version['created_at']}
                    package_version.append(vp)

            version_for_del = get_version_for_remove(package_version)
            print("count of versions of the package to delete - ", len(version_for_del))
            for version in version_for_del:
                print("delete id=", version['version_id'])
                api.packages.delete_package_version_for_org(org=org_name, package_name=package['name'],
                                                            package_type=package['package_type'],
                                                            package_version_id=version['version_id'])

print("finish")
