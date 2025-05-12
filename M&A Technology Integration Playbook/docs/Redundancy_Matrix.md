# System Redundancy & Rationalization Matrix

| Application/Service      | Microsoft                                 | LinkedIn                   | Decision                                                         |
|--------------------------|-------------------------------------------|----------------------------|------------------------------------------------------------------|
| IAM                      | Azure AD                                  | Internal IDP               | Integrate internal IDP into Azure AD B2B                        |
| Collaboration            | Teams/Exchange                            | LinkedIn Messaging         | Use Teams as primary, integrate messaging via connector          |
| CRM                      | Dynamics CRM                              | Sales Navigator            | Migrate contacts from Sales Navigator to Dynamics CRM           |
| Data Lake                | Azure Data Lake                           | Hadoop HDFS on AWS         | Consolidate to Azure Data Lake                                   |
| Compute                  | Azure VMs, AKS                            | AWS EC2, Lambda            | Maintain multi-cloud; containerize workloads                    |
| CI/CD                    | Azure DevOps, GitHub Actions              | Jenkins                    | Migrate Jenkins pipelines to GitHub Actions                     |
| Security                 | Azure Security Center, Azure Sentinel     | AWS GuardDuty              | Centralize security alerts in Azure Sentinel                    |
