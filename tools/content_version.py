"""Current readable logical pack version, independent of codec introduction versions.

Keep this standard-library-only module importable by split/package inspection.
Historical builders retain their explicit output versions and format thresholds.
The synthetic pipeline gate checks agreement with the C++ runtime boundary.
"""

MAXIMUM_CONTENT_VERSION = 43
