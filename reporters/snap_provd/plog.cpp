#include "plog.hpp"


int plog_process(FILE *log)
{
	struct provmsg header;
	struct provmsg *msg;
	unsigned int sz;
	int rv = 0;

	ProvenanceGraph graph;

	struct timeval start, end;
	float num_events=0;

	sleep(30);

	gettimeofday(&start,NULL);
	while (fread(&header, sizeof header, 1, log) == 1) {

		sz = msg_getlen(&header);
		if (sz < sizeof(header)){
		  printf("Sz < sizeof header\n");
			return EINVAL;
		}

		/*
		  printf("Msg type is %d of size %d\n",
			 (header.type),sz);
		*/

		msg = (struct provmsg *)malloc(sz);
		if (!msg){
			return ENOMEM;
		}

		sz -= sizeof(*msg);

		/* Fill in the entire message */
		*msg = header;
		if (sz > 0 && fread(msg + 1, sz, 1, log) < 1) {
			free(msg);
		  printf("Sz and fread junk\n");
			return ferror(log) ? EIO : EINVAL;
		}

		/* Process message */
		rv = graph.handle(header.type,msg);
		if(!rv)
		  num_events++;

		rv = 0;

		free(msg);
		/*
		if (rv)
			return rv;
		*/
	}

	sleep(30);
     	
	//out:	 
	gettimeofday(&end,NULL);

	/* Calculate elapsed time */

	/*
	double elapsed_time = (end.tv_sec + (double) end.tv_usec / 1000000) - (start.tv_sec + (double) start.tv_usec / 1000000 );
	printf("Elapsed time is %f seconds\n",elapsed_time);
	printf("Number of events is %f\n",num_events);
	printf("Events per second is %f\n", num_events / elapsed_time);
	printf("Number of nodes is %d\n", graph.GetNodeCount());
	printf("Number of edges is %d\n", graph.GetEdgeCount());
	*/

	/*
	FILE * file;
	file = fopen ("mygraph.txt" , "w");
	graph.Dump(file);
	fclose (file);
	*/

	/*
	std::vector<TInt> inode_ids;
        for (std::map<InodeVersion,TInt>::iterator it=graph.InodeVersionMap.begin();
             it!=graph.InodeVersionMap.end();
             ++it)   {
	  inode_ids.push_back(it->second);
	}

	double elapsed=0,min=100,max=0;
	double  total_elapsed=0;
	int total_queries=0;
        size_t total_ancestors=0, max_ancestors=0;

	for( int i=0; i < (int)inode_ids.size(); i++ ) {	  
	  int tgt_id = i;

	  gettimeofday(&start, NULL);
	  
	  std::map<int,int> ancestors = graph.RecurseAncestors(tgt_id);

	  if(ancestors.size() < 50)
	    continue;

	  if(rv < 0)
	    continue;

	  gettimeofday(&end, NULL);

	  total_queries++;

	  elapsed = (double) (end.tv_usec - start.tv_usec) / 1000000 +
	    (double) (end.tv_sec - start.tv_sec);	  

	  total_elapsed += elapsed;
	  if(elapsed > max) max = elapsed;
	  if(elapsed < min) min = elapsed;
	  total_ancestors += (int) ancestors.size();
	  if(max_ancestors < ancestors.size()) max_ancestors = ancestors.size();
	  
	  printf("DUR=%0.4fs\tAVG(DUR)=%0.4fs\tMAX(DUR)=%0.4fs\tANC=%dc\tAVG(ANC)=%0.0fc\tMAX(ANC)=%dc\n",
		 (double) elapsed,
		 (double) total_elapsed/total_queries,
		 (double) max,
		 (int)ancestors.size(),
		 (double) total_ancestors / total_queries,
		 (int)max_ancestors);
	*/

	  /*
	  bool unique = true;
	  for(int j=0; i < (int)ancestors.size(); i++) {
	    for(int k=0; j < (int)ancestors.size(); i++) { 
	      if (j == k)
		continue;
	      else if(ancestors[j] == ancestors[k])
		unique = false;
	    }
	  }
	  if(unique)
	    printf("Unique!\n");
	  else
	    printf("Not unique!\n");

	}
	  */	
	return ferror(log) ? EIO : 0;
}

int plog_process_raw(int log, ProvenanceGraph & graph)
{
	struct provmsg header;
	struct provmsg *msg;
	int n;
	size_t sz;
	int rv = 0;

	long num_events=0;

	while ( (n = TEMP_FAILURE_RETRY(read(log, &header, sizeof header))) > 0) {
		sz = msg_getlen(&header);
		if (sz < sizeof(header)){
		  fprintf(stderr,"In %s, error and the error is EINVAL\n",__func__);
		  return EINVAL;
		}

		msg = (struct provmsg *)malloc(sz);
		if (!msg){
		  fprintf(stderr,"In %s, error and the error is ENOMEM\n",__func__);
		  return ENOMEM;
		}

		sz -= sizeof(*msg);

		//Fill in the entire message
		*msg = header;
		rv=-1;
		if (sz > 0 && (rv=read(log, msg + 1, sz)) < 1) {
		  fprintf(stderr,"In %s, error parsing msg type %d\n",__func__,msg->type);
		  goto keep_going_anyway;
		  //fprintf(stderr,"Error and the error is %d\n",rv);
		}


		/* Process message */
		rv = graph.handle(header.type,msg);
		if(!rv)
		  num_events++;

		if(rv){
		  /* At this time I do not care if the message is not handled */
		  //return rv;
		}

	keep_going_anyway:
		free(msg);
		rv = 0;

	}

	return rv;
}

void stat_test(ProvenanceGraph graph){
  FILE * file;
  struct stat st;
  file = fopen ("/tmp/mygraph.txt" , "w");
  graph.Dump(file);
  fstat(fileno(file),&st);
  fclose (file);
  
  file = fopen ("/tmp/prov_log_size.plot" , "a");
  fprintf(file,"%ld\n",st.st_size);
  fclose(file);
}

	/*

	TStrV attrs,values;
	TInt EdgeId;
	TNEANet::TEdgeI tgt;

	TNEANet::TNodeI BootNode = graph.Graph->GetNI(0);
	printf("Boot node has %d incoming edges and %d outgoing edges\n",
	       BootNode.GetInDeg(),BootNode.GetOutDeg());

	BootNode.GetAttrNames(attrs); 
	BootNode.GetAttrVal(values); 
	for(int i=0; i < attrs.Len(); i++){
	  printf("\t\t%s: %s\n",attrs[i].CStr(),values[i].CStr());
	}

	for(int i=0; i<BootNode.GetInDeg(); i++){
	  EdgeId = BootNode.GetInEId(i);
	  printf("EID=%d\n",EdgeId);
	  tgt = graph.Graph->GetEI(EdgeId);
	  printf("%d -> %d\n",tgt.GetSrcNId(),tgt.GetDstNId());
	  
	  tgt.GetStrAttrNames(attrs); 
	  tgt.GetStrAttrVal(values); 
	  for(int i=0; i < attrs.Len(); i++){
	    printf("\t\t%s: %s\n",attrs[i].CStr(),values[i].CStr());
	  }
	}

	for(int i=0; i<BootNode.GetOutDeg(); i++){
	  EdgeId = BootNode.GetOutEId(i);
	  tgt = graph.Graph->GetEI(EdgeId);
	  printf("%d -> %d\n",tgt.GetSrcNId(),tgt.GetDstNId());
	  
	  tgt.GetStrAttrNames(attrs); 
	  tgt.GetStrAttrVal(values); 
	  for(int i=0; i < attrs.Len(); i++){
	    printf("\t\t%s: %s\n",attrs[i].CStr(),values[i].CStr());
	  }
	}
	*/

	/*
	//TStrV attrs,values;
	for(int i=0; i< graph.Graph->GetMxEId(); i++){
	  TNEANet::TEdgeI Edge = graph.Graph->GetEI(i);
	  Edge.GetStrAttrNames(attrs);;
	  Edge.GetStrAttrVal(values);
	  printf("\t%d -> %d\n",Edge.GetSrcNId(),Edge.GetDstNId());
	  printf("\t%s: %s\n",attrs[0].CStr(),values[0].CStr());
	}
	*/
