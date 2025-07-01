Set up the HPC for multiple users:

You likely have an NFS, so make some directories inside of it for each users with special permissions so others can't access their files.

In jobs.json there will be a user tag as well so ktrack can filter by user

Directory structure:
\shared
	- KlaudScheduler (has jobs.json and other important files)
	- \home
		- \alice
		- \bob
