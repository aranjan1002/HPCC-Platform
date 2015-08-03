The aim of this documentation is to conserve my internship experiences and serve as an informative source for someone 
who is just starting to learn the details of HPCC systems from scratch.

The overall goal of my intersnhip is to improve the efficiency of execution of child queries. What is a child query and how can we go about improving it is explained in later parts of the document.

#Before the internship
Since, I applied for this using Google Summer of Code, I had to develop a good idea about the project to develop a proposal. The first goal was to get an understanding of what HPCC system is and how does it work. The links on this page does this job pretty well:

    http://hpccsystems.com/community/training-videos
    
I went through all the links under these topics as they were very much related to my work. 
* HPCC
* ECL 
* Installing HPCC
In brief, HPCC is a platform to execute powerful queries on huge amounts of data (also called Big Data). The queries are written in an in-house declarative language called ECL. 

HPCC is a direct competitor of MapReduce. A major advantage that HPCC has over MapReduce is that its query language is much more powerful and allows a variety of complex operations. On the other hand, it is also a bit of a drawback, because the user needs to understand how to use ECL and its various tools which may be a bit of a learning curve.

Quite often I came across the terms: thor, hthor and roxie. I found it best to consider them as different physical clusters of computers. They vary in the way they execute certain aspects of an ECL query. It is my guess that thor is used for large datasets and roxie for small ones.

The next step was to understand some ECL queries and see how it worked. Each ECL query is first converted to xml and c++ code. The execution engine uses these two execute the query. Gavin sent me some basic example queries. In addition, the following two links helped me quite a lot:

https://github.com/hpcc-systems/HPCC-Platform/blob/master/ecl/eclcc/DOCUMENTATION.rst
http://hpccsystems.com/download/docs/ecl-language-reference/html

The first link is to understand some fine details in the processing of an ECL query. Now that I look back at it, a lot more things seem to make sense compared to how it looked when I initially read it. The second link is useful to understand any particular line in the ECL code. Since, there could be many syntax of any give operation, this may get confusing. It is good to think of them as overloaded functions. 



Before getting to know HPCC, I have worked on mySQL and Oracle. In one of my courses, we had to develop a new database from scrach based on some code framework in C++. Until then, my view of a database system limited to some classic algorthms to execute database operations like select, sort, join etc and how to write functional queries in sql to get these done. The internship helped me explore to some extent a system in which all these features are already there. My work was to make some small tweaks and hope to have a favorable effect at the end.

