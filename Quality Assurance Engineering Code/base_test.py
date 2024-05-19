from locust import HttpUser, SequentialTaskSet, between, task
import uuid
X_API_KEY = "a0790f26-e39e-40e7-946b-39b0bbe900ca"
EMAIL_PREFIX = "perftest"


class UserBehavior(SequentialTaskSet):

    token = ""
    accountID = None
    shedAccountId = None
    customerID = None
    cardID = None
    cardNumber = None
    cardsCreated = False
    def __init__(self, parent):
        super().__init__(parent)
        self.emailId = parent.emailId

    @task
    def verify_user(self):
        jsonData  = {
            "email":self.emailId,
            "otp":"819489"
        }
        self.client.post("/api/auth/verifyOtp",json=jsonData, headers={"x-api-key": X_API_KEY},name="Verify User")

    @task
    def create_customer(self):
        if not self.cardsCreated:
            jsonData  = {
                "firstName":"PK201",
                "middleName":"S",
                "lastName":"99",
                "dateOfBirth":"2023-09-11",
                "mobileNumber":"0224200397",
                "gender":"Male",
                "addressLine1":"2 WARHORSE GATES",
                "addressLine2":"HALLSWELL",
                "city":"CHRISTCHURCH",
                "postcode":"8026",
                "accountType":10,
                "country":"NZD",
                "isPhysical":True,
                "color":"Black",
                "irdNumber":"094-334-268",
                "taxRate":"28"
            }
            self.client.post("/api/customer/",json=jsonData, headers={"x-api-key": X_API_KEY, 'Authorization':f"Bearer {self.emailId}"},name="Create Customer")

    @task
    def get_customer(self):
        resp = self.client.get("/api/customer/", headers={"x-api-key": X_API_KEY,  'Authorization':f"Bearer {self.emailId}"},name="Get Customer")
        self.accountID = resp.json().get("records").get("accounts")[0].get("accountID")
        self.customerID = resp.json().get("records").get("customerID")
    @task
    def create_card(self):
        if not self.cardsCreated:
            jsonData  = {
                "isPhysical":True,
                "color":"#FDFEFE",
                "cardName":"second Card",
                "isName":True
            }
            self.client.post(f"/api/customer/{self.accountID}/cards/",json=jsonData, headers={"x-api-key": X_API_KEY,  'Authorization':f"Bearer {self.emailId}"},name="Create card")
    
    @task
    def get_payment_session(self):
        self.client.get(f"/api/payments/account/{self.accountID}/sessions?amount=10.0", headers={"x-api-key": X_API_KEY,  'Authorization':f"Bearer {self.emailId}"},name="Get Payment Session")

    
    @task
    def create_virtual_card(self):
        if not self.cardsCreated:
            jsonData  = {
                "isPhysical":False,
                "color":"#FDFEFE",
                "cardName":"second Card",
                "isName":True
            }
            resp = self.client.post(f"/api/customer/{self.accountID}/cards/",json=jsonData, headers={"x-api-key": X_API_KEY,  'Authorization':f"Bearer {self.emailId}"},name="Create Virtual Card")
            self.cardID = resp.json().get("records")[0].get("cardID")
            self.cardNumber = resp.json().get("records")[0].get("cardNumber")
    @task
    def update_virtual_card_limits(self):
        if not self.cardsCreated:
            jsonData  = {
                "isPhysical":False,
                "color":"#FDFEFE",
                "cardName":"second Card",
                "isName":True
            }
            self.client.put(f"/api/customer/card/{self.cardID}",json=jsonData, headers={"x-api-key": X_API_KEY,  'Authorization':f"Bearer {self.emailId}"},name="Update Virtual Card Limits")
            self.cardsCreated = True

    
    @task
    def inAppProvisioning(self):
        jsonData  = {
            "accountPanSuffix":f"{self.cardNumber}",
            "infoObject":"cardInfo",
            "certificates":"",
            "nonce":"",
            "nonceSignature":""
        }
        self.client.post("/api/customer/inAppProvision",json=jsonData, headers={"x-api-key": X_API_KEY,  'Authorization':f"Bearer {self.emailId}"},name="In App Provisioning")
    
    @task
    def resetCardPin(self):
        jsonData  = {
            "pin":"1234"
        }
        self.client.post(f"/api/customer/account/{self.accountID}/card/{self.cardID}/pin",json=jsonData, headers={"x-api-key": X_API_KEY,  'Authorization':f"Bearer {self.emailId}"},name="Reset Card Pin")
    
    @task
    def BlockCard(self):
        jsonData  = {
            
        }
        self.client.put(f"/api/customer/card/{self.cardID}/block",json=jsonData, headers={"x-api-key": X_API_KEY,  'Authorization':f"Bearer {self.emailId}"},name="Block Card")
     
    @task
    def UnBlockCard(self):
        jsonData  = {
            
        }
        self.client.put(f"/api/customer/card/{self.cardID}/unblock",json=jsonData, headers={"x-api-key": X_API_KEY,  'Authorization':f"Bearer {self.emailId}"},name="UnBlock Card")
    
    @task
    def GetCardDetails(self):
        
        self.client.get(f"/api/customer/card/details/{self.cardID}",headers={"x-api-key": X_API_KEY,  'Authorization':f"Bearer {self.emailId}"},name="Get Card Details")

    @task
    def getTransactions(self):
        jsonData = {
            "page":1,
            "perPage":30
        }
        self.client.post("/api/transactions",json=jsonData,headers={"x-api-key": X_API_KEY,  'Authorization':f"Bearer {self.emailId}"},name="Get Transactions")
    
    # @task
    # def updateTransactions(self):
    #     jsonData  = {
    #         "accountID":103,
    #         "merchant":"Raman lal chikki",
    #         "categoryID":7,
    #         "notes":"Khai chiikki 100$ ki"
    #     }
    #     self.client.put("/api/transactions/422/",json=jsonData,headers={"x-api-key": X_API_KEY,  'Authorization':f"Bearer {self.emailId}"},name="Update Transaction")

    @task
    def getSummary(self):
        jsonData = {
            "startDate":"2024-03-01",
            "endDate":"2024-03-30",
            "type":"money_out"
        }
       
        self.client.post("/api/transactions/summary",json=jsonData,headers={"x-api-key": X_API_KEY,  'Authorization':f"Bearer {self.emailId}"},name="Get Summary")
    
    @task
    def getSummaryCat1(self):
        jsonData = {
            "startDate":"2023-10-01",
            "endDate":"2023-10-05"
        }
        self.client.post("/api/transactions/summary/category/1",json=jsonData,headers={"x-api-key": X_API_KEY,  'Authorization':f"Bearer {self.emailId}"},name="Get Summary Category 1")
    
    @task
    def getSummaryCat2(self):
        jsonData = {
            "startDate":"2024-10-01",
            "endDate":"2024-10-05"
        }
        self.client.post("/api/transactions/summary/category/1",json=jsonData,headers={"x-api-key": X_API_KEY,  'Authorization':f"Bearer {self.emailId}"},name="Get Summary Category 2")
    
    @task
    def updateBudget(self):
        jsonData = {
            "budget":10
        }
        self.client.put("/api/transactions/category/1",json=jsonData,headers={"x-api-key": X_API_KEY,  'Authorization':f"Bearer {self.emailId}"},name="Update Budget")
    
    @task
    def createShed(self):
        jsonData = {
            "name":"PERFSHED",
            "goal":1200.00,
            "roundToNearest":10,
            "image":"base64encodedImage",
            "recommendationId": 1,
            "growth":True
        }

        resp = self.client.post("/api/savings/shed",json=jsonData,headers={"x-api-key": X_API_KEY,  'Authorization':f"Bearer {self.emailId}"},name="Create Shed")
        self.shedID = resp.json().get("record").get("accountID")
    
    @task
    def TrasnfertoShed(self):
        jsonData = {
            "fromAccount":self.accountID,
            "toAccount":self.shedAccountId,
            "amount":1000,
            "reference":"PERF Transfer",
            "code":"PERF Transfer",
            "particulars":"PERF Transfer"
        }

        self.client.post("/api/transactions/transfer",json=jsonData,headers={"x-api-key": X_API_KEY,  'Authorization':f"Bearer {self.emailId}"},name="Transfer to Shed")
    

    @task
    def getmobilelist(self):
        jsonData = {
            "mobiles": [
                {
                    "id":1,
                    "phoneNumber":"0224200397"
                },
                {
                    "id":1,
                    "phoneNumber":"+64224200397"
                }
            ]
        }

        self.client.post("/api/customer/mobile/list",json=jsonData,headers={"x-api-key": X_API_KEY,  'Authorization':f"Bearer {self.emailId}"},name="Get Mobile List 100")
    
    @task
    def createPIN(self):
        jsonData = {
            "pin":"12345"
        }

        self.client.post("/api/customer/pin",json=jsonData,headers={"x-api-key": X_API_KEY,  'Authorization':f"Bearer {self.emailId}"},name="create customer pin")
    

class MyUser(HttpUser):
    wait_time = between(2, 4)
    host = "https://uat-debutapp.orangeglacier-87f03b6d.australiaeast.azurecontainerapps.io"
    
    emailId = f"{EMAIL_PREFIX}+{str(uuid.uuid4())}@gmail.com"
    tasks = [UserBehavior]
    
    def on_start(self):
        self.client.post("/api/auth/register", json={"email": self.emailId}, headers={"X-API-Key":X_API_KEY}, name="Register User")
    