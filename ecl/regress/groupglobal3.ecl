/*##############################################################################

    HPCC SYSTEMS software Copyright (C) 2012 HPCC Systems.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
############################################################################## */

#option ('targetClusterType', 'roxie');

r1 := { unsigned i };
r2 := { unsigned i, dataset(r1) ii; };
r3 := { unsigned i, dataset(r2) jj; };

ds1a := dataset([3,4],r1);
ds1b := dataset([6,7],r1);
ds2a := dataset([{2,ds1a},{5,ds1b}],r2);
ds3 := dataset([{1,ds2a}],r3);

gr := GROUP(nofold(ds3),  i) : global;


f(unsigned x) := sort(project(gr, transform(recordof(gr), self.i := left.i + x; self := left)), i);
d2 := dataset([1,2], { unsigned val });
p := project(d2, transform({ unsigned cnt, dataset(recordof(gr)) cd; }, SELF.cnt := count(table(f(left.val), { count(group) })); self.cd := ungroup(f(left.val))));

output(dataset([{gr}], { dataset(recordof(gr)) cd; } ));

