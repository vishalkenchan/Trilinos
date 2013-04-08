#include <stk_adapt/IEdgeAdapter.hpp>

namespace stk {
  namespace adapt {

    void IEdgeAdapter::
    refineMethodApply(NodeRegistry::ElementFunctionPrototype function, const stk::mesh::Entity element,
                                            vector<NeededEntityType>& needed_entity_ranks)
    {
      const CellTopologyData * const cell_topo_data = stk::percept::PerceptMesh::get_cell_topology(element);

      CellTopology cell_topo(cell_topo_data);
      const mesh::PairIterRelation elem_nodes = element.relations(stk::mesh::MetaData::NODE_RANK);

      VectorFieldType* coordField = m_eMesh.get_coordinates_field();

      for (unsigned ineed_ent=0; ineed_ent < needed_entity_ranks.size(); ineed_ent++)
        {
          unsigned numSubDimNeededEntities = 0;
          stk::mesh::EntityRank needed_entity_rank = needed_entity_ranks[ineed_ent].first;

          if (needed_entity_rank == m_eMesh.edge_rank())
            {
              numSubDimNeededEntities = cell_topo_data->edge_count;
            }
          else if (needed_entity_rank == m_eMesh.face_rank())
            {
              numSubDimNeededEntities = cell_topo_data->side_count;
              throw std::runtime_error("IEdgeAdapter::apply can't use IEdgeAdapter for RefinerPatterns that require face nodes");
            }
          else if (needed_entity_rank == stk::mesh::MetaData::ELEMENT_RANK)
            {
              numSubDimNeededEntities = 1;
              throw std::runtime_error("IEdgeAdapter::apply can't use IEdgeAdapter for RefinerPatterns that require volume nodes");
            }

          // see how many edges are already marked
          int num_marked=0;
          std::vector<int> edge_marks(numSubDimNeededEntities,0);
          if (needed_entity_rank == m_eMesh.edge_rank())
            {
              for (unsigned iSubDimOrd = 0; iSubDimOrd < numSubDimNeededEntities; iSubDimOrd++)
                {
                  bool is_empty = m_nodeRegistry->is_empty( element, needed_entity_rank, iSubDimOrd);
                  if (!is_empty)
                    {
                      edge_marks[iSubDimOrd] = 1;
                      ++num_marked;
                    }
                }
            }

          for (unsigned iSubDimOrd = 0; iSubDimOrd < numSubDimNeededEntities; iSubDimOrd++)
            {
              if (needed_entity_rank == m_eMesh.edge_rank())
                {
                  stk::mesh::Entity node0 = elem_nodes[cell_topo_data->edge[iSubDimOrd].node[0]].entity();
                  stk::mesh::Entity node1 = elem_nodes[cell_topo_data->edge[iSubDimOrd].node[1]].entity();
                  double * const coord0 = stk::mesh::field_data( *coordField , node0 );
                  double * const coord1 = stk::mesh::field_data( *coordField , node1 );


                  int markInfo = mark(element, iSubDimOrd, node0, node1, coord0, coord1, &edge_marks);

                  bool needNodes = (DO_REFINE & markInfo);
                    {
                      (m_nodeRegistry ->* function)(element, needed_entity_ranks[ineed_ent], iSubDimOrd, needNodes);
                    }
                }

            } // iSubDimOrd
        } // ineed_ent
    }

    int IEdgeAdapter::markCountRefinedEdges(const stk::mesh::Entity element)
    {
      const CellTopologyData * const cell_topo_data = stk::percept::PerceptMesh::get_cell_topology(element);

      CellTopology cell_topo(cell_topo_data);
      const mesh::PairIterRelation elem_nodes = element.relations(stk::mesh::MetaData::NODE_RANK);

      VectorFieldType* coordField = m_eMesh.get_coordinates_field();

      unsigned numSubDimNeededEntities = 0;
      numSubDimNeededEntities = cell_topo_data->edge_count;

      int ref_count=0;
      for (unsigned iSubDimOrd = 0; iSubDimOrd < numSubDimNeededEntities; iSubDimOrd++)
        {
          stk::mesh::Entity node0 = elem_nodes[cell_topo_data->edge[iSubDimOrd].node[0]].entity();
          stk::mesh::Entity node1 = elem_nodes[cell_topo_data->edge[iSubDimOrd].node[1]].entity();
          double * const coord0 = stk::mesh::field_data( *coordField , node0 );
          double * const coord1 = stk::mesh::field_data( *coordField , node1 );

          int markInfo = mark(element, iSubDimOrd, node0, node1, coord0, coord1, 0);
          bool do_ref = markInfo & DO_REFINE;
          if (do_ref)
            {
              ++ref_count;
            }
        }
      return ref_count;
    }

    int IEdgeAdapter::markUnrefine(const stk::mesh::Entity element)
    {
      const CellTopologyData * const cell_topo_data = stk::percept::PerceptMesh::get_cell_topology(element);

      CellTopology cell_topo(cell_topo_data);
      const mesh::PairIterRelation elem_nodes = element.relations(stk::mesh::MetaData::NODE_RANK);

      VectorFieldType* coordField = m_eMesh.get_coordinates_field();

      unsigned numSubDimNeededEntities = 0;
      numSubDimNeededEntities = cell_topo_data->edge_count;

      bool unrefAllEdges = true;
      for (unsigned iSubDimOrd = 0; iSubDimOrd < numSubDimNeededEntities; iSubDimOrd++)
        {
          stk::mesh::Entity node0 = elem_nodes[cell_topo_data->edge[iSubDimOrd].node[0]].entity();
          stk::mesh::Entity node1 = elem_nodes[cell_topo_data->edge[iSubDimOrd].node[1]].entity();
          double * const coord0 = stk::mesh::field_data( *coordField , node0 );
          double * const coord1 = stk::mesh::field_data( *coordField , node1 );

          int markInfo = mark(element, iSubDimOrd, node0, node1, coord0, coord1, 0);
          bool do_unref = markInfo & DO_UNREFINE;
          if (!do_unref)
            {
              unrefAllEdges = false;
              break;
            }
        }
      if (unrefAllEdges)
        return -1;
      else
        return 0;
    }

    ElementUnrefineCollection IEdgeAdapter::buildUnrefineList()
    {
      ElementUnrefineCollection elements_to_unref;

      const vector<stk::mesh::Bucket*> & buckets = m_eMesh.get_bulk_data()->buckets( stk::mesh::MetaData::ELEMENT_RANK );

      for ( vector<stk::mesh::Bucket*>::const_iterator k = buckets.begin() ; k != buckets.end() ; ++k )
        {
          {
            stk::mesh::Bucket & bucket = **k ;

            const unsigned num_entity_in_bucket = bucket.size();
            for (unsigned ientity = 0; ientity < num_entity_in_bucket; ientity++)
              {
                stk::mesh::Entity element = bucket[ientity];

                // FIXME
                // skip elements that are already a parent (if there's no family tree yet, it's not a parent, so avoid throwing an error is isParentElement)
                const bool check_for_family_tree = false;
                bool isParent = m_eMesh.isParentElement(element, check_for_family_tree);

                if (isParent)
                  continue;

                const mesh::PairIterRelation elem_nodes = element.relations(stk::mesh::MetaData::NODE_RANK);

                if (elem_nodes.size() && m_eMesh.isChildWithoutNieces(element, false))
                  {
                    int markInfo = markUnrefine(element);
                    if (markInfo & DO_UNREFINE)
                      {
                        elements_to_unref.insert(element);
                        //std::cout << "tmp unref element id= " << element.identifier() << std::endl;
                        //m_eMesh.print_entity(std::cout, element);
                      }
                  }
              }
          }
        }

      return elements_to_unref;
    }

  }
}
