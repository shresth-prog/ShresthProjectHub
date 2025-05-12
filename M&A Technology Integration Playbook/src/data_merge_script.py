#!/usr/bin/env python3
"""
Merge Microsoft and LinkedIn user profiles for M&A integration.
"""

import pandas as pd

# Load data
ms = pd.read_csv('microsoft_profiles.csv')
li = pd.read_csv('linkedin_profiles.csv')

# Merge on Email
merged = pd.merge(ms, li, on='Email', how='outer', suffixes=('_MS', '_LI'))

# Combine and clean
merged['Name'] = merged['Name_MS'].combine_first(merged['Name_LI'])
merged['Role'] = merged['Department'].combine_first(merged['Title'])
merged['Location'] = merged['Location'].combine_first(merged['Office'])
final = merged[['Email', 'Name', 'Role', 'Location']]

print("Unified User Profiles Sample:")
#print(final.head(10).to_string(index=False))
print(final.to_string(index=False))
