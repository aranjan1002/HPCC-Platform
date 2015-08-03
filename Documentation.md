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

The first link is to understand some fine details in the processing of an ECL query. Now that I look back at it, a lot more things seem to make sense compared to how it looked when I initially read it. The second link is useful to understand any particular line in the ECL code. Since, there could be many syntax of any given operation, this may get confusing. It is good to think of them as overloaded functions. 

With this I found that the best way to identify a child query is to look for the element: child="1" in the generated xml graph. Via email, I was told that what are the potential problems in the generation of child queries. I tried to come up with ECL queries to create those problems but was unable to do so. Eventually, Gavin sent me a bunch of examples ecl codes: gsoc1 to gsoc6, which explored the relevant problems. The ecl files can be found here:

https://github.com/hpcc-systems/HPCC-Platform/tree/master/testing/regress/ecl

Before getting to know HPCC, I have worked on mySQL and Oracle. In one of my courses, we had to develop a new database from scrach based on some C++ code framework. Until then, my view of a database system limited to some classic algorthms to execute database operations like select, sort, join etc and how to write functional queries in sql to get these done. The internship helped me explore to some extent a system in which all these features are already there. My work was to make some small tweaks and hope to have a favorable effect at the end. 

#During the internship
I posted my weekly updates in the duration of internship here:

https://aranjan1002.wordpress.com/2015/07/26/6/

I would go through some important details during my experiences as an intern. 

## Code Generator
A major part of the internship was to understand the process of generating C++ and xml codes from an ECL query. The first step is converion of an ECL query into an expression graph. This expression graph consists of all the operations required to execute the query. Next, it is decided that which activity (or bunch of them) should be execute in a child query and which should be done inline. The function canProcessInline plays a major role in this decision. After this, some optimization are applied to the activities in the child graph. At the end the inline activities and the c++ implementation of childgraph activities are combined in c++ file. 

## Feedback
At the end of internship I have the following questions/suggestions:

1. What exactly is the structure of an xml graph. I see that there is a graph within a graph within a node and many other combinations. Some graph nodes look redundant and appear just as another rectangle in the graph representation. 
2. I think I was not able to use graph control in firefox. Though it was generating a graph it was not the same as Jamie saw in his own instance. Is there some instuction on how to use it with firefox?
2. I saw that some child queries have another child query inside it. Is that usual and is there any advantage of having that?
3. I do not have a complete understanding of Owned, Link and setClear etc. I think this needs a proper documentation with good examples.
4. I was caught in the error: Graph context not found. Is there a good way to print the value of the variable ctx?
5. I have quite a few confusions when it comes to writing ECL queries. For example, I am not sure what does ROW operation do. In the documentation it says it creates a single data record. Does it mean it creates a single record with one column? Or something else?
6. I had some trouble understaing the NORMALIZE operation. It seemed that all it did is perform certain transfrom operation n number of times. If there is a normalize operation which does the traditional normalization of tables, what kind of normalization it does (2nf, 3nf)?
6. When I dump an Ihqlexpression, I see a lot of information which I cannot quite understand. What does %e or %t represent on the left side of each line?
7. What does an alias operator do? It seems to have a significant role but I am not sure what that is.
8. It will be a good idea to have a documentation for each node_operator. Are they all associated with an activity? 
9. I have not understood the role of hash in sorting.
10. If we have a documentation of all operators in node_operator, it will also be good to associate each of them with the proper url of the documentation here: http://hpccsystems.com/download/docs/ecl-language-reference/html
11. I think it would be a great source of understanding ECL if each of the operators explained in here (http://hpccsystems.com/download/docs/ecl-language-reference/html) also contains examples of how it behaves on a given dataset. For example, we have an example dataset on which we can do these operation, and what changes are made with execution of each of these operations. There should be examples for each use of the given operator.
12. I see that there a few terms which mean the same thing. For examle dataset and recordset and table (I guess?), field and column, operation and activity. If possible, it would be better address them as one unique term.
13. Many similar topics here (http://hpccsystems.com/download/docs/ecl-language-reference/html) point to the same url. For example: Apply and Apply function. I am not sure whether that serves any purpose.
14. It would probably be a good idea to generate an error if an option is not available but used in command line. For example if I use something like -fanyrandomthing, it does not generate an error.

There are certain things which I believe were really valuable experiences for me during the internship. Getting to know how to work with github was a great addition to my resume. 
