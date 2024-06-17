#include <iostream>
#include <fstream>
#include "json.hpp"
#include <time.h>

using json=nlohmann::json;
using namespace std;

class driver{
        public:
        int index;
        vector<pair<int,int>> timewindows;
        int trailer;
        int minInterSHIFTDURATION;
        int TimeCost;
};

class trailer{
    public:
    int index;
    int Capacity;
    int InitialQuantity;
    int DistanceCost;
};


class Base{
    public:
    int index;
};

class Source{
    public:
    int index;
    int setupTime;
};

class customer{
    public:
    int index;
    int setupTime;
    vector<int> allowedTrailers;
    vector<int> Forecast;
    vector<int> Forecast_24h;
    int Capacity;
    int InitialTankQuantity;
    int SafetyLevel;
    int DeliveredQuantity=0;

};

vector<vector<int>> timeMatrix;
vector<vector<int>> dist;
vector<driver> drivers;
vector<trailer> trailers;
vector<customer> customers;
Base base;
Source source;

class Trip{
    public:
    vector<pair<int,int>> trip;
    int trailer;
    int cost;
    int capacity;
    int time=0;
    int distance=0;
    int totalquantity=0;
    void print(){
        cout<<trailer<<" :";
        for(pair<int,int> p : trip) cout<<" ("<<p.first<<","<<p.second<<")";
        //cout<<" C="<<capacity; //afficher la quantité restante dans le camion
        //cout<<" T="<<time;  //afficher la durée du trip
        cout<<endl;

    }

    

    void merge(){
       
       for(int i=1;i<trip.size();i++){
            if(trip[i].first==trip[i-1].first && i>1){
                trip[i-1].second+=trip[i].second;
                trip.erase(trip.begin() + i);
                i--;
            }
       }
        
    }

    void time_calc(){
        time=0;
        merge();
        int j;
            for(int i=1;i<trip.size();i++){
                j=trip[i].first;
                time+=timeMatrix[j][trip[i-1].first];
                
                if(j>1) time+=customers[j].setupTime;
                if(j=1) time+=source.setupTime;
            }
        }

    void dist_calc(){
        for(int i=1;i<trip.size();i++){
            distance+=dist[trip[i].first][trip[i-1].first];
        }
    }

    void cost_calc(){
        dist_calc();
        time_calc();
        for(driver D : drivers){
            if (D.trailer == trailer) {
                cost = distance*trailers[trailer].DistanceCost + time*D.TimeCost;
                return;
            }
        }
    }

    void quantity_calc(){
        for(pair<int,int> p : trip){
            totalquantity+= p.second;
        }
    }
};

class Solution{
    public:
    vector<vector<Trip>> trips;
    int cout;
    int totalquantity;

};

int unit;
int horizon;


vector<tuple<int,int,int>> demands_daily_update(int day);

pair<int,int> insertion_cost(Trip T, tuple<int,int,int> demand);

tuple<int,int,int> best_insertion_cost(vector<Trip> Trips,vector<tuple<int,int,int>> CL);

vector<tuple<int,int,int>> demands_earliest_day();

int main(int argc,char** argv){

    if(argc!=4){
        cout<<"paramètres requis : <alpha> <itérations> <instance JSON>"<<endl;
        return 1;
    }

    

    //debut parsing
    string path = argv[3];
    ifstream f(path);
    json data = json::parse(f);

    unit = data.value("unit",-1);
    horizon = data.value("horizon",-1);
    

    for (auto& i : data["drivers"]["IRP_Roadef_Challenge_Instance_driver"]){
        driver D;
        D.index = i.value("index",-1);
        D.trailer = i.value("trailer",-1);
        D.minInterSHIFTDURATION = i.value("minInterSHIFTDURATION",-1);
        D.TimeCost = i.value("TimeCost",-1);
        vector<pair<int,int>> tw;
        for (auto& j : i["timewindows"]["TimeWindow"]){
            pair<int,int> p;
            p.first = j.value("start",-1);
            p.second = j.value("end",-1);
            tw.push_back(p);
        }
        D.timewindows = tw;
        drivers.push_back(D);
    }

    for (auto& i : data["trailers"]["IRP_Roadef_Challenge_Instance_Trailers"]){
        trailer T;
        T.index = i.value("index",-1);
        T.Capacity = i.value("Capacity",-1);
        T.InitialQuantity = i.value("InitialQuantity",-1);
        T.DistanceCost = i.value("DistanceCost",-1);
        trailers.push_back(T);
    }

    base.index = data["bases"].value("index",-1);
    source.index = data["sources"]["IRP_Roadef_Challenge_Instance_Sources"].value("index",-1);
    source.setupTime = data["sources"]["IRP_Roadef_Challenge_Instance_Sources"].value("setupTime",-1);

    for (auto& i : data["customers"]["IRP_Roadef_Challenge_Instance_Customers"]){
        customer C;
        C.index = i.value("index",-1);
        C.setupTime = i.value("setupTime",-1);
        vector<int> aT;
        for (auto &t : i["allowedTrailers"]["int"])
            aT.push_back(t);
        C.allowedTrailers=aT;
        vector<int> F;
        for (auto &f : i["Forecast"]["double"])
            F.push_back(f);
        C.Forecast=F;
        C.Capacity = i.value("Capactity",-1);
        C.InitialTankQuantity = i.value("InitialTankQuantity",-1);
        C.SafetyLevel = i.value("SafetyLevel",-1);
        customers.push_back(C);
    }

    for (auto& i : data["DistMatrices"]["ArrayOfDouble"]){
        vector<int> tmp;
        for (auto& j : i["double"])
            tmp.push_back(j);
        dist.push_back(tmp);
    }

    for (auto& i : data["timeMatrices"]["ArrayOfInt"]){
        vector<int> tmp;
        for (auto& j : i["int"])
            tmp.push_back(j);
        timeMatrix.push_back(tmp);
    }
    //fin parsing
    
    vector<Solution> Solutions;

    int  alpha=atoi(argv[1]);
    int iter=atoi(argv[2]);






    int k; //compteur
    int s; //somme forecast sur 24h

    for (customer &C : customers){
        k=0;
        s=0;
        for(int f : C.Forecast){
            k++;
            s+=f;
            if (k%24==0){
                C.Forecast_24h.push_back(s);
                s=0;
            }
        }
    }

    clock_t start = clock();

    cout<<"S\tcout\tquantité\tratio"<<endl;
    for(int w=0;w<iter;w++){

        for(customer &C : customers){
            C.DeliveredQuantity=0;
        }  

        Solution S;
        vector<tuple<int,int,int>> CL;
        vector<tuple<int,int,int,int,int>> RCL;
        
        tuple<int,int,int> best; //index demande + index trip + index position
        int totalcost=0;
        int totalquantity=0;
        for (int day=0;day<horizon/24;day++){

            vector<Trip> dailytrips;
            S.trips.push_back(dailytrips);
            CL = demands_daily_update(day);
            
            
            for (trailer Tr : trailers){
                Trip T;
                T.trip.push_back(make_pair(source.index,0));
                T.trip.push_back(make_pair(source.index,0));
                T.trailer=Tr.index;
                T.capacity=trailers[T.trailer].Capacity;
                S.trips[day].push_back(T);
            }

            while(!CL.empty()){
            //calculer la demande avec le cout d'insertion le plus faible
                RCL.clear();

                for(int i=0;i<alpha;i++){
                    if(CL.empty()) break;
                                
                    best = best_insertion_cost(S.trips[day],CL);
                    /*if(get<0>(best) == -1) {  //normalement inutile
                        CL.erase(CL.begin() + get<0>(best));
                        cout<<"erreur capacité"<<endl;
                        break;
                    }*/

                    RCL.push_back(make_tuple(get<0>(CL[get<0>(best)]),get<1>(CL[get<0>(best)]),get<2>(CL[get<0>(best)]),get<1>(best),get<2>(best)));
                    
                    CL.erase(CL.begin() + get<0>(best));
                }

                //choisir aleatoirement dans RCL et inserer
                srand(w*time(NULL));
                int random = rand()%RCL.size();

                S.trips[day][get<3>(RCL[random])].trip.insert(S.trips[day][get<3>(RCL[random])].trip.begin()+ 1 + get<4>(RCL[random]), make_pair(customers[get<0>(RCL[random])].index,get<2>(RCL[random])));
                S.trips[day][get<3>(RCL[random])].capacity -= get<2>(RCL[random]);
                customers[get<0>(RCL[random])].DeliveredQuantity += get<2>(RCL[random]);
            
                CL = demands_daily_update(day);
            }

            //step2
            if(day==horizon/24-1) break;

            CL = demands_earliest_day();
        
            while(!CL.empty()){

                RCL.clear();

                for(int i=0;i<alpha;i++){
                    if(CL.empty()) break;

                    best = best_insertion_cost(S.trips[day],CL);
                    if(get<0>(best) != -1) {

                    RCL.push_back(make_tuple(get<0>(CL[get<0>(best)]),get<1>(CL[get<0>(best)]),get<2>(CL[get<0>(best)]),get<1>(best),get<2>(best)));
                    CL.erase(CL.begin() + get<0>(best));
                    }else CL.clear();

                }

                if(RCL.size()>0){
                    int random = rand()%RCL.size();

                    S.trips[day][get<3>(RCL[random])].trip.insert(S.trips[day][get<3>(RCL[random])].trip.begin()+ 1 + get<4>(RCL[random]), make_pair(customers[get<0>(RCL[random])].index,get<2>(RCL[random])));
                    S.trips[day][get<3>(RCL[random])].capacity -= get<2>(RCL[random]);
                    customers[get<0>(RCL[random])].DeliveredQuantity += get<2>(RCL[random]);
                    RCL.erase(RCL.begin() + random);
                    
                }
                CL = demands_earliest_day();

                //size check
                bool b=false;
                for(auto dmd : CL){
                    for (auto trp : S.trips[day]){
                        if(get<2>(dmd)<trp.capacity){
                            b=true;
                            break;
                        }
                    }
                    if (b) break;
                }
                if (b==false) CL.clear();
                        
            }

            for(auto &m : S.trips[day]){
                m.cost_calc();
                totalcost+=m.cost;
                m.quantity_calc();
                totalquantity+=m.totalquantity;
            }
        }
        S.cout=totalcost;
        S.totalquantity=totalquantity;
        cout<<w+1<<"\t"<<totalcost<<"\t"<<totalquantity<<"\t\t"<<(float)totalcost/(float)totalquantity<<endl;
        Solutions.push_back(S);

    }

    clock_t end = clock();
    double temps = double(end - start)/CLOCKS_PER_SEC;
    
    int minS=0,Sindex=0;
    float ratio;
    for (Solution s : Solutions){
        
        if((float)s.cout/(float)s.totalquantity<(float)Solutions[minS].cout/(float)Solutions[minS].totalquantity){
            minS=Sindex;
        }
        Sindex++;
    }

    cout<<endl<<"meilleur : S"<<minS<<"   "<<(float)Solutions[minS].cout/(float)Solutions[minS].totalquantity<<endl;

    for(int day=0;day<horizon/24;day++){
        cout<<"jour "<<day+1<<endl;
        for(auto m : Solutions[minS].trips[day]){  
            m.print(); 
        }

    }
    cout<<"temps : "<<temps<<"s"<<endl;
    return 0;
}

//mise a jour des demandes

vector<tuple<int,int,int>> demands_daily_update(int currentday){
    vector<tuple<int,int,int>> demands;
    int estimation;
    int index_client=0;
    int conso;
    int q,max=0;

    for (customer C : customers){
        conso=0;
        for(int day=0;day<=currentday;day++){
            conso+=C.Forecast_24h[day];
        }
        estimation = C.InitialTankQuantity + C.DeliveredQuantity - conso;
        
        if (estimation < C.SafetyLevel){
            q=C.SafetyLevel - estimation;

            for(trailer tr : trailers) max=tr.Capacity>max?tr.Capacity:max;

            if(q>max){
                demands.push_back(make_tuple(index_client,currentday,q-max));
                demands.push_back(make_tuple(index_client,currentday,max));
            }else
                demands.push_back(make_tuple(index_client,currentday,q));
        }
        index_client++;
    }
    return demands;
};

vector<tuple<int,int,int>> demands_earliest_day(){
    vector<tuple<int,int,int>> demands;
    int index_client=0;
    int estimation;
    int day;
    bool b;
    int q,max=0;
    for (customer C : customers){
        b=false;
        day=0;
        estimation = C.InitialTankQuantity + C.DeliveredQuantity;
 
        while(b==false && day<(horizon/24)){
            estimation -= C.Forecast_24h[day];
            
            if (estimation < C.SafetyLevel){
                b=true;
                q=C.SafetyLevel-estimation;
                for (trailer tr : trailers) max=tr.Capacity>max?tr.Capacity:max;
                if (q>max){
                    demands.push_back(make_tuple(index_client,day,max));
                    demands.push_back(make_tuple(index_client,day,q-max));
                }else
                demands.push_back(make_tuple(index_client,day,C.SafetyLevel-estimation));
                
            }
            day++;
        }
        index_client++;

    }
    return demands;
};

//renvoie la demande la moins chere a inserer, dans quel trip, a quelle position
tuple<int,int,int> best_insertion_cost(vector<Trip> Trips,vector<tuple<int,int,int>> CL){
    pair<int,int> ins;
    int mincost=1000000;
    int mintrip;
    int pos;
    int minindex=-1; 
    int i;
    for(Trip T : Trips){
        i=0;
        for(tuple<int,int,int> demand : CL){
            if(T.capacity >= get<2>(demand)){
                ins = insertion_cost(T,demand);
                
                if (ins.first < mincost){
                    mincost=ins.first;
                    pos=ins.second;
                    mintrip=T.trailer;
                    minindex=i;

                }
                //cout<<get<0>(demand)<<" "<<ins.first<<" | "<<minindex<<" "<<mincost<<endl;
                
            }
            i++;
        }
    }

    if(minindex==-1) return make_tuple(-1,-1,-1);
    return make_tuple(minindex,mintrip,pos);
};

//renvoie le cout d'insertion minimum et la position
pair<int,int> insertion_cost(Trip T, tuple<int,int,int> demand){
    int d0,d1,cost;
    int index_client = customers[get<0>(demand)].index;
    int mincost=1000000;
    int minindex;
    for (int i=0;i<T.trip.size()-1;i++){
        d0 = dist[T.trip[i].first][T.trip[i+1].first];
        d1 = dist[T.trip[i].first][index_client]+dist[index_client][T.trip[i+1].first];
        cost = d1-d0;
        if (cost<mincost){
            mincost=cost;
            minindex=i;
        }
    }

    //cout<<"calcul de "<<get<0>(demand)<<" dans "<<T.trailer<<" : "<<mincost<<" a la pos "<<minindex<<endl;
    return make_pair(mincost,minindex);
};
