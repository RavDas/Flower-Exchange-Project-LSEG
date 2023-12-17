#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <iterator>
#include <chrono>
#include <queue>
#include <sstream>
#include <set> 


using namespace std;

class Order {
private:
    static std::set<std::string> clientOrderIdSet;
public:
    static int currentOrderId;
    string orderId;
    string clientOrderId;
    string instrument;
    int side;
    double price;
    int quantity;
    int remQuantity;
    int status;
    int priority;
    string reason;

    Order() {}

    Order(string *_clientOrderId, string *_instrument, string *_side, string *_quantity, string *_price);

    void checkOrder(string *_clientOrderId, string *_instrument, string *_side, string *_quantity, string *_price);
    bool checkValidity();
    void execute(ofstream&, int);
    void execute(ofstream&);
};

/* in buy orders, order with higher price gets higher priority
   in sell orders, order with lower price gets lower priority
   in both order types, if 
*/
struct Compare {
    bool operator() (Order ord1, Order ord2) {
        if (ord1.side == 1) {
            return ord1.price < ord2.price;
        } else {
            return ord1.price > ord2.price;
        }
    }
};

int Order::currentOrderId = 1;
std::set<std::string> Order::clientOrderIdSet;

Order::Order(string *_clientOrderId, string *_instrument, string *_side, string *_quantity, string *_price) {
    orderId = "ord" + to_string(currentOrderId++);
    checkOrder(_clientOrderId, _instrument, _side, _quantity, _price);
    clientOrderId = *_clientOrderId;
    instrument = *_instrument;
    side = stoi(*_side);
    price = stod(*_price);
    quantity = stoi(*_quantity);
    remQuantity = quantity;
};


bool Order::checkValidity() {
    if (status == 1)
        return false;
    return true;
};

string instruments[5] = {"Rose","Lavender","Lotus","Tulip","Orchid"};   

void Order::checkOrder(string *_clientOrderId, string *_instrument, string *_side, string *_quantity, string *_price) {
    if (!((*_clientOrderId).length() && (*_instrument).length() && (*_side).length() && (*_price).length() && (*_quantity).length())) {
        reason = "Invalid fields";
        status = 1;
        return;
    }
    if ((*_clientOrderId).length() > 7) {
        cerr << "Error: Client Order ID " << *_clientOrderId << " exceeds 7 characters" << endl;
        exit(1); 
    }
    // Check for repeating client order IDs
    if (clientOrderIdSet.find(*_clientOrderId) != clientOrderIdSet.end()) {
        cerr << "Error: Repeating Client Order ID " << *_clientOrderId << endl;
        exit(1); 
    } else {
        clientOrderIdSet.insert(*_clientOrderId);
    }
    if (find(begin(instruments), end(instruments), *_instrument) == end(instruments)) {
        reason = "Invalid instrument";
        status = 1;
        return;
    }
    int s = stoi(*_side);
    if (!(s == 1 || s == 2)) {
        reason = "Invalid side";
        status = 1;
        return;
    }
    double p = stod(*_price);
    if (p <= 0.0) {
        reason = "Invalid price";
        status = 1;
        return;
    }
    int q = stoi(*_quantity);
    if (q % 10 != 0 || q < 10 || q > 1000) {
        reason = "Invalid size";
        status = 1;
        return;
    }
    reason = "";
    status = 0;
};

void Order::execute(ofstream &fout, int execQuantity) {
    string stat = "";
    if (status == 0) stat = "New";
    else if (status == 1) stat = "Reject";
    else if (status == 2) stat = "Fill";
    else if (status == 3) stat = "PFill";

    // Get the current timestamp
    auto now = chrono::system_clock::now();
    auto in_time_t = chrono::system_clock::to_time_t(now);
    auto milliseconds = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;

    // Format the timestamp
    tm *time_info = localtime(&in_time_t);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y%m%d-%H%M%S", time_info);
    
    // Check the length of the reason field
    if (reason.length() > 50) {
        cerr << "Error: Reason exceeds 50 characters" << endl;
        exit(1);
    }
    fout << orderId << "," << clientOrderId << "," << instrument << "," << side << "," << stat << "," 
         << execQuantity << "," << fixed << setprecision(2) << price << ","
         << reason << "," << time_str << "." << setfill('0') << setw(3) << milliseconds.count() << "\n";

};

void Order::execute(ofstream &fout) {
    execute(fout, remQuantity);
};

int main(void) {

    priority_queue<Order, vector<Order>, Compare> roseBuy;
    priority_queue<Order, vector<Order>, Compare> roseSell;
    priority_queue<Order, vector<Order>, Compare> lavenderBuy;
    priority_queue<Order, vector<Order>, Compare> lavenderSell;
    priority_queue<Order, vector<Order>, Compare> lotusBuy;
    priority_queue<Order, vector<Order>, Compare> lotusSell;
    priority_queue<Order, vector<Order>, Compare> tulipBuy;
    priority_queue<Order, vector<Order>, Compare> tulipSell;
    priority_queue<Order, vector<Order>, Compare> orchidBuy;
    priority_queue<Order, vector<Order>, Compare> orchidSell;

    // Abstraction of priority_queues
    priority_queue<Order, vector<Order>, Compare> *orderBooks[5][2]= {
        {&roseBuy, &roseSell},
        {&lavenderBuy, &lavenderSell},
        {&lotusBuy, &lotusSell},
        {&tulipBuy, &tulipSell},
        {&orchidBuy, &orchidSell}
    };

    // I/O file handling
    ifstream fin;
    fin.open("order.csv", ios::in);
    ofstream fout;
    fout.open("execution_rep.csv", ios::out);
    fout << "Order ID,Client Order ID,Instrument,Side,Exec Status,Quantity,Price,Reason,Timestamp\n";

    vector<string> row;
    string line, word, temp; 

    int count = 0;
    while(getline(fin, line)) {
        if(++count == 1) 
            continue;  
        row.clear();
        stringstream s(line);
        
        while(getline(s, word, ',')) {
            row.push_back(word);
        }

        // Constructing a new order 
        Order newOrder(&row[0], &row[1], &row[2], &row[3], &row[4]);

        if(newOrder.checkValidity()) {
            // Get the index of the instrument 
            int index = (int)(find(begin(instruments), end(instruments), newOrder.instrument) - begin(instruments));
            priority_queue<Order, vector<Order>, Compare>** orderBook = orderBooks[index];
            
            // consider buy side of the instrument
            if(newOrder.side == 1) {
                // sell side of the instrument not empty and new order price >= top selling price
                while(!(orderBook[1]->empty()) && ((orderBook[1]->top()).price <= newOrder.price)) {  // sell side of the instrument not empty and new order price >= top selling price
                    Order topOrder = (orderBook[1]->top());
                    if (newOrder.remQuantity == topOrder.remQuantity) {
                        newOrder.status = 2;
                        topOrder.status = 2;
                        newOrder.price = topOrder.price;
                        newOrder.execute(fout);
                        topOrder.execute(fout);
                        newOrder.remQuantity = 0;
                        topOrder.remQuantity = 0;
                        orderBook[1] -> pop();   
                        break;
                        
                    } else if (newOrder.remQuantity > topOrder.remQuantity) {
                        double tempPrice = newOrder.price;
                        newOrder.status = 3;
                        topOrder.status = 2;
                        newOrder.price = topOrder.price;
                        newOrder.execute(fout, topOrder.remQuantity);
                        topOrder.execute(fout);
                        newOrder.remQuantity = newOrder.remQuantity - topOrder.remQuantity;
                        topOrder.remQuantity = 0;
                        newOrder.price = tempPrice;
                        orderBook[1]->pop();
                         
                    } else {
                        newOrder.status = 2;
                        topOrder.status = 3;
                        newOrder.price = topOrder.price;
                        newOrder.execute(fout);
                        topOrder.execute(fout, newOrder.remQuantity);
                        topOrder.remQuantity = topOrder.remQuantity - newOrder.remQuantity;
                        orderBook[1] -> pop();  
                        orderBook[1] -> emplace(topOrder);  
                        newOrder.remQuantity = 0;
                        break;
                    }
                }

                if (newOrder.status == 0) {
                    newOrder.execute(fout);
                }
                
                if(newOrder.remQuantity > 0.0) {
                    orderBook[0]->emplace(newOrder);
                }
            //consider sell side of the instrument
            } else if (newOrder.side == 2) {
                // buy side of the instrument not empty and new order price <= top selling price
                while(!(orderBook[0]->empty()) && ((orderBook[0]->top()).price >= newOrder.price)) { 
                    Order topOrder = (orderBook[0]->top());
                    if (newOrder.remQuantity == topOrder.remQuantity) {
                        newOrder.status = 2;
                        topOrder.status = 2;
                        newOrder.price = topOrder.price;
                        newOrder.execute(fout);
                        topOrder.execute(fout);
                        newOrder.remQuantity = 0;
                        topOrder.remQuantity = 0;
                        orderBook[0] -> pop(); 
                        break;

                    } else if (newOrder.remQuantity > topOrder.remQuantity) {
                        double tempPrice = newOrder.price;
                        newOrder.status = 3;
                        topOrder.status = 2;
                        newOrder.price = topOrder.price;
                        newOrder.execute(fout, topOrder.remQuantity);
                        newOrder.remQuantity = newOrder.remQuantity - topOrder.remQuantity;
                        topOrder.execute(fout);
                        topOrder.remQuantity = 0;
                        newOrder.price = tempPrice;
                        orderBook[0] -> pop(); 

                    } else {
                        newOrder.status = 2;
                        topOrder.status = 3;
                        newOrder.price = topOrder.price;
                        newOrder.execute(fout);
                        topOrder.execute(fout, newOrder.remQuantity);
                        topOrder.remQuantity = topOrder.remQuantity - newOrder.remQuantity;
                        orderBook[0] -> pop();  
                        orderBook[0] -> emplace(topOrder); 
                        newOrder.remQuantity = 0;  
                        break;
                    }
                }

                if (newOrder.status == 0) {
                    newOrder.execute(fout);
                }

                if(newOrder.remQuantity > 0.0) {
                    orderBook[1]->emplace(newOrder);
                }
            }
        } else {
            newOrder.execute(fout);
        }
    }
    //close files
    fin.close();
    fout.close();

    return 0;
}


