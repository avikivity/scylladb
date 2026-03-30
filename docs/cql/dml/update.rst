.. highlight:: cql

.. _update-statement:

UPDATE
^^^^^^

Updating a row is done using an ``UPDATE`` statement:

.. code-block::

   update_statement: UPDATE `table_name`
                   : [ USING `update_parameter` ( AND `update_parameter` )* ]
                   : SET `assignment` ( ',' `assignment` )*
                   : WHERE `where_clause`
                   : [ IF ( EXISTS | `condition` ( AND `condition` )*) ]
   update_parameter: ( TIMESTAMP `int_value` | TTL  `int_value` | TIMEOUT `duration` )
   int_value: ( `integer` | `bind_marker` )
   assignment: `simple_selection` '=' `term`
             : | `column_name` '=' `column_name` ( '+' | '-' ) `term`
             : | `column_name` '=' `list_literal` '+' `column_name`
    simple_selection: `column_name`
                    : | `column_name` '[' `term` ']'
                    : | `column_name` '.' `field_name`
                    : | '(' '?' `native_type` ')' `simple_selection`
    condition: `simple_selection` `operator` `term`

For instance:

.. code-block:: cql

    UPDATE NerdMovies USING TTL 400
       SET director   = 'Joss Whedon',
           main_actor = 'Nathan Fillion',
           year       = 2005
     WHERE movie = 'Serenity';

    UPDATE UserActions
       SET total = total + 2
       WHERE user = B70DE1D0-9908-4AE3-BE34-5573E5B09F14
         AND action = 'click';

The ``UPDATE`` statement writes one or more columns for a given row in a table. The :token:`where_clause` is used to
select the row to update and must include all columns composing the ``PRIMARY KEY``. Non-primary key columns are then
set using the ``SET`` keyword.

Note that unlike in SQL, ``UPDATE`` does not check the prior existence of the row by default, 

(except through IF_, see below): 

the row is created if none existed before, and updated otherwise. Furthermore, there is no way to know
whether creation or update occurred.

In an ``UPDATE`` statement, all updates within the same partition key are applied atomically,
meaning either all provided values are stored or none at all.

Similarly to ``INSERT``, ``UPDATE`` statement happening concurrently at different
cluster nodes proceed without coordination. Cell values
supplied by a statement with the highest timestamp will prevail.
If two ``UPDATE`` statements or ``UPDATE`` and ``INSERT``
statements have the same timestamp, a conflict resolution algorithm determines which cells prevails
(see :ref:`update ordering <update-ordering>`).

Regarding the :token:`assignment`:

- ``c = c + 3`` is used to increment/decrement counters. The column name after the '=' sign **must** be the same as
  the one before the '=' sign. Note that increment/decrement is only allowed on counters, and are the *only* update
  operations allowed on counters. See the section on :ref:`counters <counters>` for details.
- ``id = id + <some-collection>`` and ``id[value1] = value2`` are for collections, see the :ref:`relevant section
  <collections>` for details.
- ``id.field = 3`` is for setting the value of a field on non-frozen user-defined types. 

Please refer to the :ref:`update parameters <update-parameters>` section for more information on the :token:`update_parameter`.

.. _IF:

IF condition
~~~~~~~~~~~~

It is, however, possible to use the conditions on some columns through ``IF``, in which case the row will not be updated
unless the conditions are met. Each such ``UPDATE`` gets a globally unique timestamp.
But, please note that using ``IF`` conditions will incur a non-negligible performance
cost (internally, Paxos will be used), so this should be used sparingly.

JSON field selection in IF conditions
'''''''''''''''''''''''''''''''''''''

Columns of the ``json`` type (see :ref:`native types <native-types>`) can be used in ``IF`` conditions with the
``(?type)`` field selection syntax. This allows lightweight transactions to compare individual fields within a BSON
document against expected values.

The syntax mirrors the ``(?type)`` selector used in ``SELECT`` (see :ref:`selection clause <selection-clause>`), but
applied within an ``IF`` condition::

    UPDATE t SET v = 1 WHERE pk = 1 IF (?int)doc.age = 42
    UPDATE t SET v = 1 WHERE pk = 1 IF (?text)doc.address.city = 'London'
    UPDATE t SET v = 1 WHERE pk = 1 IF (?int)doc.scores[0] = 100

If the field path does not exist or the value is not compatible with the requested type, the result is ``NULL``.
You can test for missing fields using ``IF (?int)doc.field = null``.


:doc:`Apache Cassandra Query Language (CQL) Reference </cql/index>`

.. include:: /rst_include/apache-copyrights.rst


.. Licensed to the Apache Software Foundation (ASF) under one
.. or more contributor license agreements.  See the NOTICE file
.. distributed with this work for additional information
.. regarding copyright ownership.  The ASF licenses this file
.. to you under the Apache License, Version 2.0 (the
.. "License"); you may not use this file except in compliance
.. with the License.  You may obtain a copy of the License at
..
..     http://www.apache.org/licenses/LICENSE-2.0
..
.. Unless required by applicable law or agreed to in writing, software
.. distributed under the License is distributed on an "AS IS" BASIS,
.. WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
.. See the License for the specific language governing permissions and
.. limitations under the License.
